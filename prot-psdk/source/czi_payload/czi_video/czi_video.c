#include "czi_video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dji_platform.h"
#include <unistd.h>
#include <stdbool.h>
#include "dji_liveview.h"
#include "dji_logger.h"
#include "dji_platform.h"
#include "dji_aircraft_info.h"
#include "time.h"
#include <pthread.h>
#include "../czi_protocol_handler/protocol_longFormatHandler.h"
#include "../czi_transmission/czi_transmission.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define REQUEST_I_FRAME_INTERVAL 2
#define VIDEO_BUFFER_SIZE (7 * 1024 * 1024)
#define SHM_NAME "/video_stream_shm"

static pthread_mutex_t streamMutex = PTHREAD_MUTEX_INITIALIZER;
static bool stopStreamFlag = false;
static bool initialized = false;
static uint8_t *circularBuffer = NULL;
static uint32_t bufferOffset = 0;
static uint32_t totalBytesWritten = 0;
static pthread_mutex_t action_mutex = PTHREAD_MUTEX_INITIALIZER;
static int action = 0;
static bool isStreaming = false;
static bool videoThreadRunning = false;
static void* videoStreamThreadFunc(void* arg);

T_DjiReturnCode CziVideo_Init(void) 
{
    if (initialized) {
        printf("CziVideo already initialized.\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    T_DjiReturnCode returnCode = DjiLiveview_Init();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("Failed to initialize DjiLiveview: %d\n", returnCode);
        return returnCode;
    }

    printf("DjiLiveview initialized successfully.\n");

    // 创建共享内存
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        printf("Failed to create shared memory: %s\n", strerror(errno));
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (ftruncate(shm_fd, VIDEO_BUFFER_SIZE) == -1) {
        printf("Failed to set size of shared memory: %s\n", strerror(errno));
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    circularBuffer = mmap(0, VIDEO_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (circularBuffer == MAP_FAILED) {
        printf("Failed to map shared memory: %s\n", strerror(errno));
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    close(shm_fd);
    initialized = true;

    FILE* file = fopen(DETECT_STATE_FILE_PATH, "w");
    if (file != NULL) {
        fclose(file);
    } else {
        printf("Failed to open %s for writing\n", DETECT_STATE_FILE_PATH);
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode CziVideo_Deinit(void) 
{
    if (!initialized) {
        printf("CziVideo not initialized.\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    T_DjiReturnCode returnCode = DjiLiveview_Deinit();

    if (circularBuffer != NULL) {
        munmap(circularBuffer, VIDEO_BUFFER_SIZE);
        shm_unlink(SHM_NAME);
        circularBuffer = NULL;
    }

    initialized = false;

    return returnCode;
}

static void H264DataCallback(E_DjiLiveViewCameraPosition position, const uint8_t *buf, uint32_t len) 
{
    pthread_mutex_lock(&streamMutex);

    if (bufferOffset + len > VIDEO_BUFFER_SIZE) {
        // Wrap around and start overwriting old data
        uint32_t spaceLeft = VIDEO_BUFFER_SIZE - bufferOffset;
        memcpy(circularBuffer + bufferOffset, buf, spaceLeft);
        memcpy(circularBuffer, buf + spaceLeft, len - spaceLeft);
        bufferOffset = len - spaceLeft;
        totalBytesWritten = VIDEO_BUFFER_SIZE;
    } else {
        memcpy(circularBuffer + bufferOffset, buf, len);
        bufferOffset += len;
        totalBytesWritten += len;
        if (totalBytesWritten > VIDEO_BUFFER_SIZE) {
            totalBytesWritten = VIDEO_BUFFER_SIZE;
        }
    }

    pthread_mutex_unlock(&streamMutex);
}

T_DjiReturnCode CziVideo_StartStream(E_DjiLiveViewCameraPosition position, E_DjiLiveViewCameraSource source) 
{
    if (isStreaming) {
        printf("Stream already started.\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }
    printf("Starting H264 stream. Position: %d, Source: %d\n", position, source);
    T_DjiReturnCode ret = DjiLiveview_StartH264Stream(position, source, H264DataCallback);
    if (ret == DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        isStreaming = true;
    }
    return ret;
}

T_DjiReturnCode CziVideo_StopStream(E_DjiLiveViewCameraPosition position, E_DjiLiveViewCameraSource source) 
{
    if (!isStreaming) {
        printf("Stream not started.\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }
    printf("Stopping H264 stream. Position: %d, Source: %d\n", position, source);
    T_DjiReturnCode ret = DjiLiveview_StopH264Stream(position, source);
    if (ret == DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        isStreaming = false;
    }
    return ret;
}

T_DjiReturnCode CziVideo_RequestIFrame(E_DjiLiveViewCameraPosition position, E_DjiLiveViewCameraSource source) 
{
    printf("Requesting I-frame. Position: %d, Source: %d\n", position, source);
    return DjiLiveview_RequestIntraframeFrameData(position, source);
}

static E_DjiMountPosition GetCameraMountPosition() 
{
    T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo;
    T_DjiReturnCode returnCode;

    returnCode = DjiAircraftInfo_GetBaseInfo(&aircraftInfoBaseInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("Failed to get aircraft base info\n");
        return DJI_MOUNT_POSITION_UNKNOWN;
    }

    return aircraftInfoBaseInfo.mountPosition;
}
T_DjiReturnCode CziVideo_RunSample(int actionValue) 
{
    static pthread_t videoStreamThread;
    static bool threadCreated = false;
    static CziVideoThreadArgs *threadArgs = NULL;

    if (!threadCreated) {
        threadArgs = malloc(sizeof(CziVideoThreadArgs));
        if (!threadArgs) {
            printf("Failed to allocate memory for thread arguments\n");
            return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
        }
        threadArgs->mountPosition = DJI_MOUNT_POSITION_PAYLOAD_PORT_NO1;
        threadArgs->source = DJI_LIVEVIEW_CAMERA_SOURCE_DEFAULT;

        if (pthread_create(&videoStreamThread, NULL, videoStreamThreadFunc, threadArgs) != 0) {
            printf("Failed to create video stream thread\n");
            free(threadArgs);
            return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
        }
        threadCreated = true;
        videoThreadRunning = true;
    }

    pthread_mutex_lock(&action_mutex);
    action = actionValue;
    pthread_mutex_unlock(&action_mutex);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static void* videoStreamThreadFunc(void* arg) 
{
    printf("进入视频流获取线程\n");
    CziVideoThreadArgs *threadArgs = (CziVideoThreadArgs *)arg;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_DjiReturnCode returnCode;
    int start_state = 0;

    while (videoThreadRunning) {
        pthread_mutex_lock(&action_mutex);
        int current_action = action;
        bool current_isStreaming = isStreaming;
        pthread_mutex_unlock(&action_mutex);

        if (current_action == 1 && !current_isStreaming) {
            printf("确认为视频流开启状态\n");
            if (!current_isStreaming) {
                    returnCode = CziVideo_StartStream(threadArgs->mountPosition, threadArgs->source);
                    start_state = 1;
                    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                        printf("Failed to start H264 stream\n");
                        pthread_exit(NULL);
                    }
            }

            while (current_action == 1 && videoThreadRunning && !current_isStreaming) {
                printf("正在获取I帧数据 current_cation is %d\n",current_action);
                returnCode = CziVideo_RequestIFrame(threadArgs->mountPosition, DJI_LIVEVIEW_CAMERA_SOURCE_DEFAULT);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    printf("Failed to request I-frame\n");
                }
                
                usleep(100000);  // 等待一段时间再请求下一帧

                pthread_mutex_lock(&action_mutex);
                current_action = action;
                pthread_mutex_unlock(&action_mutex);
            }
            
            if (current_isStreaming) {
                returnCode = CziVideo_StopStream(DJI_MOUNT_POSITION_PAYLOAD_PORT_NO1, DJI_LIVEVIEW_CAMERA_SOURCE_DEFAULT);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    printf("Failed to stop H264 stream\n");
                } else {
                    printf("H264 stream stopped successfully\n");
                }
            }

        } else if (current_action == 0) {
            if (current_isStreaming) {
                returnCode = CziVideo_StopStream(DJI_MOUNT_POSITION_PAYLOAD_PORT_NO1, DJI_LIVEVIEW_CAMERA_SOURCE_DEFAULT);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    printf("Failed to stop H264 stream\n");
                } else {
                    printf("H264 stream stopped successfully\n");
                }
            }
        }
        
        usleep(100000);  // Sleep for a short period to avoid busy-waiting
    }

    printf("线程退出\n");
    free(threadArgs);  // Free the memory when the thread exits
    return NULL;
}


