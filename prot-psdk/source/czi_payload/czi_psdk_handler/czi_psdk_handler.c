#include "dji_logger.h"
#include "dji_platform.h"
#include "dji_error.h"
#include "dji_aircraft_info.h"

#include "../czi_transmission/czi_transmission.h"
#include "../czi_protocol_handler/protocol_longFormatHandler.h"
#include "../../dji_module/widget_czi_speaker/widget_czi_speaker.h"
#include "czi_psdk_handler.h"
#include "czi_megaphone/czi_megaphone.h"
#include "../czi_msdk_handler/msdk_protobuf.pb-c.h"
#include "../../dji_module/widget/widget.h"
#include "../../psdk_config.h"
#include "../czi_video/czi_video.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/inotify.h>
#include "czi_json_handler/czi_json_handler.h"
#include "czi_payload.h"
#include "../czi_gimbal/czi_gimbal.h"
#include "../czi_light/czi_light.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

static T_DjiTaskHandle gs_threadId_psdkToPortHandler;
static T_DjiTaskHandle gs_threadId_psdkFromPortHandler;
static void *CziPsdkHandler_videoStreamThread(void *arg);
static void* SchedulerThread_GetTemperature(void *arg);
int CziPsdkHandler_GetTemperature();

static pthread_t videoThread;
static pthread_t TemperatureThread;
static bool videoThreadRunning = false;
static bool isStreaming = false;
static int videoStreamFlag = 0;
static int stateStreamFlag = 1;
static int gs_temperature;

float tempVal[3];


static void *PsdkTask_RecvQueueDataFromProt(void *arg);

static void *PsdkTask_SendQueueDataToProt(void *arg)
{
    T_DjiReturnCode cziStat;
    int ret;

    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    
    while (1) {
        T_CziQueueQueue queue =  { 0x00 };
        ret = CziTransmission_GetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, &queue);


        if(queue.qdata.len <= 0) {
            osalHandler->TaskSleepMs(20);
            continue;
        }

        printf("get queue from psdk to prot len:%d\n", queue.qdata.len);

        //  printf("\n\n********************send data (psdk-->prot)********************\n\n");
        // printf("size: %d , content: ", queue.qdata.len);
        // for(int i=0; i<queue.qdata.len; i++) {
        //     printf(" 0x%02X ", queue.qdata.mtext[i]);
        // }
        // printf("\n\n***************************************************************\n\n");

        cziStat = CziTransmission_SendDataByMailbox(queue.qdata.mtext, queue.qdata.len);
        if (cziStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS){
            USER_LOG_ERROR("Send package to Mailbox error.");
            continue;
        }
    }
}

T_DjiReturnCode CziPsdkHandler_Init(void)
{
    T_DjiReturnCode returnCode;
    T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo = {0x00};

    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = DjiAircraftInfo_GetBaseInfo(&aircraftInfoBaseInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("get aircraft base info error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // if (aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M300_RTK || \
    //     aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M350_RTK || \
    //     aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M30 ||      \
    //     aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M30T || 
    //     aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M3D ||
    //     aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M3TD) {

        
        if (osalHandler->TaskCreate("PsdkTask_SendQueueDataToProt", PsdkTask_SendQueueDataToProt, STACK_SIZE_PSDK_TO_PROT_HANDLER, NULL,
                                    &gs_threadId_psdkToPortHandler) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("PsdkTask_SendQueueDataToProt create error.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
        }
        CziPsdkHandler_GetTemperature();
        if (osalHandler->TaskCreate("PsdkTask_RecvQueueDataFromProt", PsdkTask_RecvQueueDataFromProt, STACK_SIZE_PROT_TO_PSDK_HANDLER, NULL,
                                    &gs_threadId_psdkFromPortHandler) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("PsdkTask_RecvQueueDataFromProt create error.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
        }
        
        
    // }
    // else {
    //     USER_LOG_ERROR("Aircraft type is not support.");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_NONSUPPORT;
    // }

    USER_LOG_INFO("CziPsdkHandler_Init Run success");

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


void CziPsdkHandler_NormalExitHandler(void)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    osalHandler->TaskDestroy(gs_threadId_psdkToPortHandler);

    USER_LOG_INFO("CziPsdkHandler_NormalExitHandler exit success");
}

static void *PsdkTask_RecvQueueDataFromProt(void *arg)
{
    T_DjiReturnCode cziStat;
    int ret;

    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

#define FILEPATH_LISTENFILE "/tmp/listen.mp3"
    FILE *fpCache = NULL;    
    while (1) {
        
        // T_CziQueueQueue queue =  { 0x00 };
        T_ProtRspData prot = {};
#define MAILBOX_MAX_LENGTH 1024
        char recvBuffer[MAILBOX_MAX_LENGTH] = {0x00};
        int recvLen = 0;

        cziStat = CziTransmission_RecvDataByMailbox(recvBuffer, &recvLen);
        if (cziStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS){
            USER_LOG_ERROR("Recv package from Mailbox error.");
            osalHandler->TaskSleepMs(20);
            continue;
        }

        if(recvLen <= 0) {
            continue;
        }
#define MAX_FILE_PATH_LEN (256)

        // printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
        // printf("***************PSDK recv buffer\n**************");
        // for(int i=0; i<recvLen; i++) {
        //     printf(" 0x%02X ", recvBuffer[i]);
        // }

        if (recvBuffer[3] == 0x2C) {
            if(recvBuffer[5] >= 32)
                recvBuffer[5] = 32;
            CziWidget_GetRecordAmp(recvBuffer[5]);
        }  
            
        Prot_LongFormatUnpack(recvBuffer, recvLen, &prot);

        switch (prot.protCmd)
        {
            case CZI_PROTOCOL_COMMON_CMD_TTS_PLAY: //0x10
            {
                if(prot.rspData[2] == 0xF1) {
                    unsigned char input_data;
                    input_data = 0x00;
                    T_ProtRspData rsp = {0x00};
                    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_TTS_CHANGE_STAT, &input_data, sizeof(input_data), &rsp);
                    ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
                }
                break;
            }
            case CZI_PROTOCOL_COMMON_CMD_MUSIC_STATE:  //0x20  diff lp20
            {
                if(prot.rspData[1] == 0xF0) {
                    printf("Here is CZI_PROTOCOL_COMMON_CMD_MUSIC_STATE!\n");
                    unsigned char input_data;
                    int mediastats = 0;
                    Payload_SetMediaInfo(mediastats);
                    CziMegaphone_StopPlay();
                    int videoStreamState = 0;
                    // CziWidget_GetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, &videoStreamState);
                    if (videoStreamState){
                        FILE *file = fopen(VIDEO_STREAM_STATE_FILE_PATH, "w");
                        if (file != NULL) {
                            time_t now = time(NULL);
                            char timeStr[64];
                            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
                            fprintf(file, "The music state has been saved in : %s at %s\n", VIDEO_STREAM_STATE_FILE_PATH, timeStr);
                            fclose(file);
                        } else {
                            printf("Failed to open %s for writing\n",VIDEO_STREAM_STATE_FILE_PATH);
                        }
                    }
                    input_data = 0xF0;
                    T_ProtRspData rsp = {0x00};
                    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY_END, &input_data, sizeof(input_data), &rsp);
                    ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
                }
                unsigned char Amp = 0x41; 
                CziWidget_GetRecordAmp(Amp);
                break;
            }
            case CZI_PROTOCOL_COMMON_CMD_SYS_VOLUME:  //0x53  setvalue  updatevolume diff
            {
                printf("!!!!!!!!!!PSDK  RECV volum 1:%d   2:%d  3:%d\n", prot.rspData[0], prot.rspData[1], prot.rspData[2]);  
                CziWidget_SetIndexValue(WIDGET_INDEX_VOLUME, prot.rspData[2]);
                CziMegaphone_UpdateVolume(prot.rspData[2]); 
               
                printf(" 0x%02X ", recvBuffer[1]);
     
                T_ProtRspData rsp = {0x00};
                unsigned char input_data[2];
                input_data[0] = CZI_PROTOCOL_COMMON_CMD_SYS_VOLUME;
                input_data[1] = prot.rspData[2];
                int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_PROTO_BUFF, (unsigned char*)input_data, sizeof(input_data), &rsp);
                ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);

                break;
            }


            case CZI_PROTOCOL_COMMON_DATA_STREAM_UP: 
            {
                
                int flag_erro = 0;
                uint32_t result = 0;
                
                // char charArry[8] = {0};  //255 255  7  




            // char arr[] = {'1', '2', '3', '4'};
            // int length = 4;
            // int num1 = char_array_to_int(arr, length);
            // int num2 = char_array_to_int(charArry, 3);
            // printf("!!!!!!!!!!NUM:%d NUM2:%d \n", num1, num2);   
printf("!!!!!!!!!!PSDK  RECV erro     len :%d 1:%d   2:%d  3:%d\n", prot.rspLen, prot.rspData[0], prot.rspData[1], prot.rspData[2]);
                
                // for(int i=0; i<8; i++)
                // {
                //     charArry[i] = prot.rspData[i+1];
                // }
                



                for (int i = 0; i < 8; ++i) {
                    result |= (prot.rspData[i+1] << (i * 8));  
                }
                printf("!!!!!!!!!!PSDK  RECV erro     len :%d  result:%d\n", prot.rspLen, result);   //02 AE  01    430
                // printf("!!!!!!!!!!result:%d \n", resulte);   //460449

                
                if (prot.rspLen > 2 && prot.rspData[0] !=0)
                {
                    // flag_erro = 1;
                    WidgetCziSpeaker_GetErro(result);
                }
                
                break;
            }
            //  case CZI_PROTOCOL_COMMON_CMD_MUSIC_MODE: //0x22  lp20wu
            // {
            //     // printf("!!!!!!!!!!PSDK MUSIC MODE %d \n", prot.rspData[2]);  
            //     CziWidget_SetLoopModel(prot.rspData[2]);
                
            //     CziMegaphone_UpdateLoopModel(prot.rspData[2]);
            //     T_ProtRspData rsp = {0x00};
            //     unsigned char input_data[2];
            //     input_data[0] = CZI_PROTOCOL_COMMON_CMD_MUSIC_MODE;
            //     input_data[1] = prot.rspData[2];
            //     int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_PROTO_BUFF, (unsigned char*)input_data, sizeof(input_data), &rsp);
            //     ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
            //     break;
            // }


            // case CZI_PROTOCOL_COMMON_CMD_GET_MEDIA_TEMPERATURE_INFO:  //0xF7 setvalue  updatevolume diff
            // {
            //     printf("!!!!!!!!!!PSDK  RECV TEMPERATURE  1:%d   2:%d  3:%d\n", prot.rspData[0], prot.rspData[1], prot.rspData[2]);  
        
            //     break;
            // }

            case CZI_PROTOCOL_COMMON_CMD_LIGHT:  //0x63  diff lp20
            {
                // CziWidget_SetLightSwitch(prot.rspData[0], prot.rspData[1]);
                T_ProtRspData rsp = {0x00};
                // printf("1111111111!!!!!!!!!!!!!PSDK LIGHT  RECV  1: %x    2: %x   3: %x   4: %x  5:%d    6:%d\n", prot.rspData[0], prot.rspData[1], prot.rspData[2], prot.rspData[3], prot.rspData[4], prot.rspData[5]);
            
                unsigned char cmdParam = prot.rspData[0];
                switch (cmdParam)
                {
                case (0xA1):
                case (0xA0):{
                    unsigned char input_data[3];
                    input_data[0] = CZI_PROTOCOL_COMMON_CMD_LIGHT;
                    input_data[1] = prot.rspData[0];
                    input_data[2] = prot.rspData[1];
                    
                    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_PROTO_BUFF, (unsigned char*)input_data, sizeof(input_data), &rsp);
                    ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
                    break;
                }
                case 0xF3:{
                    float temp = *((float*)&prot.rspData[1]);
                    // printf("temp: %f\n",temp );
                    // // float tempVal[3];
                    // printf("!!!1:%x  2：%x  3：%x\n", prot.rspData[1]);
                    memcpy(tempVal, &prot.rspData[1], 8);
                    // tempVal[0] = (float)prot.rspData[0];
                    // tempVal[1] = (float)prot.rspData[1];
                    // sleep(15);
                    // printf("主板温度: %f, 功放板温度: %f 喇叭温度：%f\n", tempVal[0], tempVal[1], tempVal[2]);
                    CziWidget_UpdateMainTempVal(tempVal[0], tempVal[1]);
                    break;
                }

                default:
                    break;
                }
                break;
            }


            case CZI_PROTOCOL_COMMON_CMD_TTS_CHANGE_STAT:
            {
                WidgetCziSpeaker_UpdateTtsRunningState(prot.rspData[0]);
                // printf("!!!!!!!!!!!!!!!!\n");
                // printf("!!!!!!!!!!!!!!\n");
                // printf("updatettsRunningState is 0x%x\n", prot.rspData[0]);
                break;
            }

            case CZI_PROTOCOL_COMMON_CMD_AI_VIDEO:
            {
                for(int i=0;i<prot.rspLen;i++){
                    printf("prot.rspData[%d] is %X\n",i,prot.rspData[i]);
                }
                printf("prot.rsplen is %d\n",prot.rspLen);
                // CziWidget_SetAiVideoStream(prot.rspData[1]);
                if (prot.rspData[1] == 0xF0 || prot.rspData[1] == 0xF2 || prot.rspData[1] == 0xF3){
                    CziMegaphone_CtrlMusicStop();
                }
                T_ProtRspData rsp = {0x00};
                unsigned char input_data[4];
                input_data[0] = CZI_PROTOCOL_COMMON_CMD_AI_VIDEO;
                input_data[1] = prot.rspData[0];
                input_data[2] = prot.rspData[1];
                input_data[3] = 0x00;
                int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_PROTO_BUFF, (unsigned char*)input_data, sizeof(input_data), &rsp);
                ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
                // CziPsdkHandler_Video();
                break;
            }
            case CZI_PROTOCOL_COMMON_CMD_ASK_INFO: //0xff
            {
                CziProtobufInfo *cziProtoInfo = czi_protobuf_info__unpack(NULL, prot.rspLen, prot.rspData);
                /*update volume*/
                int volume = cziProtoInfo->master_volume;
                printf("(%s %s LINE-%d) cziProtoInfo-master_volume:%d\n", __FILE__, __FUNCTION__, __LINE__, cziProtoInfo->master_volume);
                CziWidget_SetIndexValue(WIDGET_INDEX_VOLUME, volume);
                CziMegaphone_UpdateVolume(volume);
                 /*update lightmode*/
                // int light_switch = cziProtoInfo->alarmlight_switch;
                int light_code = cziProtoInfo->light_code;
                CziWidget_SetLightSwitch(light_code);
                /*update cyclemode*/
                int loop_model = cziProtoInfo->loop_model;
                printf("(%s %s LINE-%d) cziProtoInfo->loop_model:%d\n", __FILE__, __FUNCTION__, __LINE__, 193);
                CziWidget_SetLoopModel(loop_model);
                CziMegaphone_UpdateLoopModel(loop_model);
                /*update searchlight*/
                // int searchlight_angle = cziProtoInfo->searchlight_angle;

                int  voice = cziProtoInfo->tts_voice;
                int  speed = cziProtoInfo->tts_speed;
                int  breathlight = cziProtoInfo->breath_light;
                char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
                DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_SWITCH, WIDGET_INDEX_BREATHING_LIGHT, breathlight, &widgetCziServiceCmd);

                printf("voice is %d  speed  is %d \n",voice, speed);
                printf("breath is %d \n",breathlight);


                /*update startup mute*/
                int startup_mute = cziProtoInfo->startup_mute;
                CziWidget_SetStartupMute(startup_mute);
                printf("SN is %s\n",cziProtoInfo->sn);
                printf("Software_version is %s\n",cziProtoInfo->software_version);
                CziWidget_SendMessage(cziProtoInfo->sn,"sn");
                CziWidget_SendMessage(cziProtoInfo->software_version,"software_version");
                // czi_protobuf_info__free_unpacked();
                ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, recvBuffer, recvLen);
                break;
            }
        
        default:
            break;
        }

        ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, recvBuffer, recvLen);

    }
}

// int CziPsdkHandler_Video() {
//     CziWidget_GetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, &videoStreamFlag);
//     if (videoStreamFlag == 1) { 
//         if (!videoThreadRunning) {
//             if (pthread_create(&videoThread, NULL, CziPsdkHandler_videoStreamThread, NULL) != 0) {
//                 USER_LOG_ERROR("Failed to create video stream thread");
//                 return -1;
//             }
//             videoThreadRunning = true;
//         }
//     }
//     return 0;
// }


static void *CziPsdkHandler_videoStreamThread(void *arg) {
    T_DjiReturnCode returnCode;
    int length, i = 0;
    int fd;
    int wd1, wd2;
    char buffer[EVENT_BUF_LEN];

    printf("Initializing inotify...\n");
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        videoThreadRunning = false;
        pthread_exit(NULL);
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    printf("Adding inotify watch on %s and %s...\n", DETECT_STATE_FILE_PATH, VIDEO_STREAM_STATE_FILE_PATH);
    wd1 = inotify_add_watch(fd, DETECT_STATE_FILE_PATH, IN_MODIFY);
    if (wd1 == -1) {
        printf("Couldn't add watch to %s\n", DETECT_STATE_FILE_PATH);
        perror("inotify_add_watch");
        close(fd);
        videoThreadRunning = false;
        pthread_exit(NULL);
    }

    wd2 = inotify_add_watch(fd, VIDEO_STREAM_STATE_FILE_PATH, IN_MODIFY);
    if (wd2 == -1) {
        printf("Couldn't add watch to %s\n", VIDEO_STREAM_STATE_FILE_PATH);
        perror("inotify_add_watch");
        close(fd);
        videoThreadRunning = false;
        pthread_exit(NULL);
    }

    while (videoThreadRunning) {
        // CziWidget_GetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, &videoStreamFlag);

        if (videoStreamFlag == 1) {
            if (!isStreaming && stateStreamFlag == 1) {
                printf("353------------------------stateStreamFlag is %d-----------------\n",stateStreamFlag);
                printf("Starting video stream...\n");
                returnCode = CziVideo_RunSample(1);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    printf("Failed to run video sample, returnCode: %d\n", returnCode);
                    usleep(50000);
                    continue;
                } 
                printf("Video stream started successfully.\n");
                isStreaming = true;
            }

            length = read(fd, buffer, EVENT_BUF_LEN);
            if (length < 0) {
                if (errno != EAGAIN) {
                    perror("read");
                }
                usleep(50000);
                continue;
            }

            printf("Read %d bytes from inotify buffer\n", length);
            i = 0;
            while (i < length) {
                struct inotify_event *event = (struct inotify_event *)&buffer[i];
                if (event->wd == wd1 && (event->mask & IN_MODIFY)) {
                    printf("The file %s was modified.\n", DETECT_STATE_FILE_PATH);
                    if (isStreaming) {
                        printf("Pausing video stream...\n");
                        stateStreamFlag = 0;
                        printf("383------------------------stateStreamFlag is %d-----------------\n",stateStreamFlag);
                        returnCode = CziVideo_RunSample(0);
                        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                            printf("Failed to stop video stream, returnCode: %d\n", returnCode);
                            usleep(50000);
                            continue;
                        }
                        isStreaming = false;
                        printf("Video stream paused.\n");
                    }
                }
                if (event->wd == wd2 && (event->mask & IN_MODIFY)) {
                    printf("The file %s was modified.\n", VIDEO_STREAM_STATE_FILE_PATH);
                    if (!isStreaming) {
                        printf("Restarting video stream...\n");
                        stateStreamFlag = 1;
                        printf("399------------------------stateStreamFlag is %d-----------------\n",stateStreamFlag);
                        returnCode = CziVideo_RunSample(1);
                        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                            printf("Failed to run video sample, returnCode: %d\n", returnCode);
                        } else {
                            isStreaming = true;
                            printf("Video stream restarted successfully.\n");
                        }
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        } else {
            if (isStreaming) {
                printf("Stopping video stream...\n");
                returnCode = CziVideo_RunSample(0);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    printf("Failed to stop video stream, returnCode: %d\n", returnCode);
                    usleep(50000);
                    continue;
                }
                isStreaming = false;
                printf("Video stream stopped successfully.\n");
            }

            usleep(30000); // Avoid busy-waiting when videoStreamFlag is 0
        }
    }

    printf("Removing inotify watch and closing file descriptor...\n");
    inotify_rm_watch(fd, wd1);
    inotify_rm_watch(fd, wd2);
    close(fd);

    printf("Exiting video stream thread...\n");
    videoThreadRunning = false;
    pthread_exit(NULL);
}

int CziPsdkHandler_GetTemperature() 
{
    // CziWidget_GetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, &videoStreamFlag);

    if (pthread_create(&TemperatureThread, NULL, SchedulerThread_GetTemperature, NULL) != 0) {
        USER_LOG_ERROR("Failed to create Thread Get Temperature");
        return -1;
    }

    return 0;
}

static void* SchedulerThread_GetTemperature(void *arg)
{
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))
#define FILEPATH_REALTIME_TEMPERATURE    "/tmp/temperature_amp"

    FILE *fp = fopen(FILEPATH_REALTIME_TEMPERATURE, "wb");
    fclose(fp);
    int fd = inotify_init();
    if (fd < 0) {
        USER_LOG_ERROR("Failed to initalize inotify\n");
    }

    int ret = inotify_add_watch(fd, FILEPATH_REALTIME_TEMPERATURE, IN_CLOSE_WRITE);   // 添加监听事件类型
    if (ret == -1) {
        USER_LOG_ERROR("Failed to add file [%s] watch\n", FILEPATH_REALTIME_TEMPERATURE);
    }

    while (1) {
        char buffer[BUF_LEN];
        int bytes_read = read(fd, &buffer, BUF_LEN);
        if (bytes_read <= 0) {
            sleep(1);
            continue;
        }
        int i = 0;
        while (i < bytes_read)  {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (!(event->mask & IN_CLOSE_WRITE))  {
                i += EVENT_SIZE + event->len;
                continue;
            }

            
            FILE *fp = fopen(FILEPATH_REALTIME_TEMPERATURE, "rb");
            char temperature[16] = {0x00};
            fread(temperature, sizeof(char), sizeof(temperature), fp);
            fclose(fp);
            gs_temperature = atoi(temperature);
            // printf("************************PSDK RECV  MEDIA TEMP   gs_temperature: %d\n", gs_temperature);
            // CziStreamEncoder_HandleAircraftState(MEDIA_CMD_WRITE, &aidetectState);
            i += EVENT_SIZE + event->len;
            // printf("(%s %s LINE-%d) i: %d, mask: %d\n", __FILE__, __FUNCTION__, __LINE__, i, event->mask);
            tempVal[2] = gs_temperature;
            CziWidget_UpdateBugleTempVal(gs_temperature);
            printf("sssssssssssssstempVal[2]：%f\n", tempVal[2]);
            // sleep(15);
            // CziWidget_UpdateTempVal(tempVal[0], tempVal[1], tempVal[2]);
        }
        
    }  
}

     

