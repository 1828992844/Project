/**
 ********************************************************************
 * @file    test_widget_speaker.c
 * @brief
 *
 * @copyright (c) 2018 DJI. All rights reserved.
 *
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 *
 *********************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "widget_czi_speaker.h"
#include "dji_logger.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "utils/util_misc.h"
#include "utils/util_md5.h"
#include "dji_aircraft_info.h"
#include "widget/widget.h"
#include "../../psdk_config.h"
#include "../czi_util/fifo_buffer.h"
#include "../czi_megaphone/czi_megaphone.h"

// #define OPUS_INSTALLED  0

#ifdef CZI_OPUS

#include <opus/opus.h>

#endif

/* Private constants ---------------------------------------------------------*/
#define WIDGET_SPEAKER_TASK_STACK_SIZE          (2048)
#define WIDGET_SPEAKER_SEND_STACK_SIZE          (2048)

/*! Attention: replace your audio device name here. */
#define WIDGET_SPEAKER_USB_AUDIO_DEVICE_NAME    "alsa_output.usb-C-Media_Electronics_Inc._USB_Audio_Device-00.analog-stereo"

#define WIDGET_SPEAKER_AUDIO_OPUS_FILE_NAME     "test_audio.opus"
#define WIDGET_SPEAKER_AUDIO_PCM_FILE_NAME      "test_audio.pcm"

#define WIDGET_SPEAKER_TTS_FILE_NAME            "test_tts.txt"
#define WIDGET_SPEAKER_TTS_OUTPUT_FILE_NAME     "tts_audio.wav"
#define WIDGET_SPEAKER_TTS_FILE_MAX_SIZE        (3000)

/* The frame size is hardcoded for this sample code but it doesn't have to be */
#define WIDGET_SPEAKER_AUDIO_OPUS_MAX_PACKET_SIZE          (3 * 1276)
#define WIDGET_SPEAKER_AUDIO_OPUS_MAX_FRAME_SIZE           (6 * 960)
#define WIDGET_SPEAKER_AUDIO_OPUS_SAMPLE_RATE              (16000)
#define WIDGET_SPEAKER_AUDIO_OPUS_CHANNELS                 (1)

#define WIDGET_SPEAKER_AUDIO_OPUS_DECODE_FRAME_SIZE_8KBPS  (40)
#define WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_8KBPS     (8000)

#define WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_FOR_REAL_RECORD   (16000)
#define WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_FOR_MUSIC         (32000)

#define MAXLEN_FILENAME    (128)
#define MAXLEN_FILEPATH    (256)
// #define BUFFER_MAX_SIZE    (230 * 80)
// #define READBUF_MAX_SIZE    (80)
#define FRAME_SIZE sizeof(uint8_t)
#define ORIGINAL_PACKET_FRAMES 230    // 原始数据包中的帧数
#define NEW_PACKET_FRAMES 160// 新数据包中的帧数       
#define PACKET_FRAMES 160 * 5// 新数据包中的帧数       
#define BUFFER_DATA_SIZE (ORIGINAL_PACKET_FRAMES + NEW_PACKET_FRAMES)


static char voiceAbsolutePath[MAXLEN_FILEPATH];
int num = 0;
int count = 0;
#define DATA_COUNT 230
#define SEND_SIZE 160
#define TEMP_SIZE 160

int data[SEND_SIZE];
int tempData[TEMP_SIZE];
int dataAgain[SEND_SIZE];

int finalCount = 0;
int tempCount = 0;
int againCount = 0;

int tempUse = 0;

// int sendFailedFlag = 0;
static uint32_t errOffset = 0;

/* The speaker initialization parameters */
#define WIDGET_SPEAKER_DEFAULT_VOLUME                (30)
#define EKHO_INSTALLED                               (1)
int MediaMode;


/* Private types -------------------------------------------------------------*/

/* Private values -------------------------------------------------------------*/
static T_DjiWidgetSpeakerHandler *s_speakerHandler = {0};
static T_DjiMutexHandle s_speakerMutex = {0};
static T_DjiWidgetSpeakerState s_speakerState = {0};
static T_DjiTaskHandle s_widgetSpeakerTestThread;

static FILE *s_ttsFile = NULL;
static FILE *s_audioFile = NULL;
char buffer[160*10000];
// static FILE *buffer = NULL;
static bool s_isDecodeFinished = true;
static uint16_t s_decodeBitrate = 0;
static E_TtsRunningState s_ttsRunningState = TTS_IDLE;

unsigned char stream_start = 0xF1;
unsigned char stream_play = 0xF2;
unsigned char stream_stop = 0xF3;

/* Private functions declaration ---------------------------------------------*/
static void SetSpeakerState(E_DjiWidgetSpeakerState speakerState);
static T_DjiReturnCode GetSpeakerState(T_DjiWidgetSpeakerState *speakerState);
static T_DjiReturnCode SetWorkMode(E_DjiWidgetSpeakerWorkMode workMode);
static T_DjiReturnCode SetPlayMode(E_DjiWidgetSpeakerPlayMode playMode);
static T_DjiReturnCode StartPlay(void);
static T_DjiReturnCode StopPlay(void);
static T_DjiReturnCode SetVolume(uint8_t volume);
static T_DjiReturnCode ReceiveTtsData(E_DjiWidgetTransmitDataEvent event,
                                      uint32_t offset, uint8_t *buf, uint16_t size);
static T_DjiReturnCode ReceiveAudioData(E_DjiWidgetTransmitDataEvent event,
                                        uint32_t offset, uint8_t *buf, uint16_t size);

static void *WidgetCziSpeaker_WidgetSpeakerTask(void *arg);
static void *WidgetCziSpeaker_AgainSendData(void *arg);
static uint32_t WidgetCziSpeaker_GetVoicePlayProcessId(void);
static uint32_t WidgetCziSpeaker_KillVoicePlayProcess(uint32_t pid);
static T_DjiReturnCode WidgetCziSpeaker_DecodeAudioData(void);
static T_DjiReturnCode WidgetCziSpeaker_PlayAudioData(void);
static T_DjiReturnCode WidgetCziSpeaker_PlayTtsData(void);
static T_DjiReturnCode WidgetCziSpeaker_CheckFileMd5Sum(const char *path, uint8_t *buf, uint16_t size);
static T_CziSpeakerPlay g_cziSpeakerPlay;
static char gs_voiceFilename[MAXLEN_FILENAME];
static char g_ttsContent[2000];

/* Exported functions definition ---------------------------------------------*/
T_DjiReturnCode WidgetCziSpeaker_WidgetSpeakerStartService(T_DjiWidgetSpeakerHandler *speakerHandler)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    s_speakerHandler = speakerHandler;

    if( s_speakerHandler->GetSpeakerState == NULL ) s_speakerHandler->GetSpeakerState = GetSpeakerState;
    if(s_speakerHandler->SetWorkMode == NULL) s_speakerHandler->SetWorkMode = SetWorkMode;
    if(s_speakerHandler->StartPlay == NULL) s_speakerHandler->StartPlay = StartPlay;
    if(s_speakerHandler->StopPlay == NULL) s_speakerHandler->StopPlay = StopPlay;
    if(s_speakerHandler->SetPlayMode == NULL) s_speakerHandler->SetPlayMode = SetPlayMode;
    if(s_speakerHandler->SetVolume == NULL) s_speakerHandler->SetVolume = SetVolume;
    if(s_speakerHandler->ReceiveTtsData == NULL) s_speakerHandler->ReceiveTtsData = ReceiveTtsData;
    if(s_speakerHandler->ReceiveVoiceData == NULL) s_speakerHandler->ReceiveVoiceData = ReceiveAudioData;

    returnCode = osalHandler->MutexCreate(&s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create speaker mutex error: 0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiWidget_RegSpeakerHandler(s_speakerHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Register speaker handler error: 0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = osalHandler->MutexLock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_IDEL;
    s_speakerState.workMode = DJI_WIDGET_SPEAKER_WORK_MODE_VOICE;
    s_speakerState.playMode = DJI_WIDGET_SPEAKER_PLAY_MODE_SINGLE_PLAY;

    returnCode = osalHandler->MutexUnlock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    // returnCode = SetVolume(WIDGET_SPEAKER_DEFAULT_VOLUME);
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Set speaker volume error: 0x%08llX", returnCode);
    //     return returnCode;
    // }

    if (osalHandler->TaskCreate("user_widget_speaker_task", WidgetCziSpeaker_WidgetSpeakerTask, WIDGET_SPEAKER_TASK_STACK_SIZE,
                                NULL,
                                &s_widgetSpeakerTestThread) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Dji widget speaker test task create error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    // if (osalHandler->TaskCreate("agin send data from file", WidgetCziSpeaker_AgainSendData, WIDGET_SPEAKER_SEND_STACK_SIZE,
    //                             NULL,
    //                             &s_widgetSpeakerTestThread) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Dji widget speaker test task create error.");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    // }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode WidgetCziSpeaker_RegistPlay(T_CziSpeakerPlay *cziSpeakerPlay)
{
    if(cziSpeakerPlay->SpeakerSetTtsContent != NULL) 
        g_cziSpeakerPlay.SpeakerSetTtsContent = cziSpeakerPlay->SpeakerSetTtsContent;
    if(cziSpeakerPlay->SpeakerSetPlayInfo != NULL) 
        g_cziSpeakerPlay.SpeakerSetPlayInfo = cziSpeakerPlay->SpeakerSetPlayInfo;
    if(cziSpeakerPlay->SpeakerSendAudioInfo != NULL)
        g_cziSpeakerPlay.SpeakerSendAudioInfo = cziSpeakerPlay->SpeakerSendAudioInfo;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode WidgetCziSpeaker_UpdateVolume(uint8_t volume)
{
    s_speakerState.volume  = volume;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode WidgetCziSpeaker_UpdatePlayMode(E_DjiWidgetSpeakerPlayMode playMode)
{
    s_speakerState.playMode = playMode;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

E_DjiWidgetSpeakerPlayMode WidgetCziSpeaker_GetPlayMode(void)
{
    return s_speakerState.playMode;
}

T_DjiReturnCode WidgetCziSpeaker_UpdateState(E_DjiWidgetSpeakerState state)
{
    s_speakerState.state = state;
    USER_LOG_INFO("Set Speaker state %d", state);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

/* Private functions definition-----------------------------------------------*/
static uint32_t WidgetCziSpeaker_GetVoicePlayProcessId(void)
{
    FILE *fp;
    char cmdStr[128];
    uint32_t pid;
    int ret;

    snprintf(cmdStr, 128, "pgrep ffplay");
    fp = popen(cmdStr, "r");
    if (fp == NULL) {
        USER_LOG_ERROR("fp is null.");
        return 0;
    }

    ret = fscanf(fp, "%u", &pid);
    if (ret <= 0) {
        pid = 0;
        goto out;
    }

out:
    pclose(fp);

    return pid;
}

static uint32_t WidgetCziSpeaker_KillVoicePlayProcess(uint32_t pid)
{
    FILE *fp;
    char cmdStr[128];

    snprintf(cmdStr, 128, "kill %d", pid);
    fp = popen(cmdStr, "r");
    if (fp == NULL) {
        USER_LOG_ERROR("fp is null.");
        return 0;
    }

    pclose(fp);

    return pid;
}

static T_DjiReturnCode WidgetCziSpeaker_DecodeAudioData(void)
{
#ifdef CZI_OPUS
    FILE *fin;
    FILE *fout;
    OpusDecoder *decoder;
    opus_int16 out[WIDGET_SPEAKER_AUDIO_OPUS_MAX_FRAME_SIZE * WIDGET_SPEAKER_AUDIO_OPUS_CHANNELS];
    uint8_t cbits[WIDGET_SPEAKER_AUDIO_OPUS_MAX_PACKET_SIZE];
    int32_t nbBytes;
    int32_t err;

    /*! Attention: you can use "ffmpeg -i xxx.mp3 -ar 16000 -ac 1 out.wav" and use opus-tools to generate opus file for test */
    fin = fopen(WIDGET_SPEAKER_AUDIO_OPUS_FILE_NAME, "r");
    if (fin == NULL) {
        fprintf(stderr, "failed to open input file: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    /* Create a new decoder state. */
    decoder = opus_decoder_create(WIDGET_SPEAKER_AUDIO_OPUS_SAMPLE_RATE, WIDGET_SPEAKER_AUDIO_OPUS_CHANNELS, &err);
    if (err < 0) {
        fprintf(stderr, "failed to create decoder: %s\n", opus_strerror(err));
        goto close_fin;
    }

    fout = fopen(WIDGET_SPEAKER_AUDIO_PCM_FILE_NAME, "w");
    if (fout == NULL) {
        fprintf(stderr, "failed to open output file: %s\n", strerror(errno));
        goto close_fin;
    }

    while (1) {
        int i;
        unsigned char pcm_bytes[WIDGET_SPEAKER_AUDIO_OPUS_MAX_FRAME_SIZE * WIDGET_SPEAKER_AUDIO_OPUS_CHANNELS * 2];
        int frame_size;

        /* Read a 16 bits/sample audio frame. */
        nbBytes = fread(cbits, 1, s_decodeBitrate / WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_8KBPS *
                                  WIDGET_SPEAKER_AUDIO_OPUS_DECODE_FRAME_SIZE_8KBPS, fin);
        if (feof(fin))
            break;

        /* Decode the data. In this example, frame_size will be constant because
           the encoder is using a constant frame size. However, that may not
           be the case for all encoders, so the decoder must always check
           the frame size returned. */
        frame_size = opus_decode(decoder, cbits, nbBytes, out, WIDGET_SPEAKER_AUDIO_OPUS_MAX_FRAME_SIZE, 0);
        if (frame_size < 0) {
            fprintf(stderr, "decoder failed: %s\n", opus_strerror(frame_size));
            goto close_fout;
        }

        USER_LOG_DEBUG("decode data to file: %d\r\n", frame_size * WIDGET_SPEAKER_AUDIO_OPUS_CHANNELS);
        /* Convert to little-endian ordering. */
        for (i = 0; i < WIDGET_SPEAKER_AUDIO_OPUS_CHANNELS * frame_size; i++) {
            pcm_bytes[2 * i] = out[i] & 0xFF;
            pcm_bytes[2 * i + 1] = (out[i] >> 8) & 0xFF;
        }
        /* Write the decoded audio to file. */
        fwrite(pcm_bytes, sizeof(short), frame_size * WIDGET_SPEAKER_AUDIO_OPUS_CHANNELS, fout);
    }

    USER_LOG_INFO("Decode Finished...");
    s_isDecodeFinished = true;

decode_data_failed:
    opus_decoder_destroy(decoder);
create_decoder_failed:
    fclose(fout);
open_pcm_audio_failed:
    fclose(fin);
#endif
    return EXIT_SUCCESS;

#ifdef CZI_OPUS
close_fout:
    fclose(fout);

close_fin:
    fclose(fin);

    return EXIT_FAILURE;
#endif

}

static T_DjiReturnCode WidgetCziSpeaker_PlayAudioData(void)
{
    char cmdStr[128];

    memset(cmdStr, 0, sizeof(cmdStr));
    USER_LOG_INFO("Start Playing...");
    // snprintf(cmdStr, sizeof(cmdStr), "ffplay -nodisp -autoexit -ar 16000 -ac 1 -f s16le -i %s 2>/dev/null",
    //          WIDGET_SPEAKER_AUDIO_PCM_FILE_NAME);

    // return DjiUserUtil_RunSystemCmd(cmdStr);
}

static T_DjiReturnCode WidgetCziSpeaker_PlayTtsData(void)
{
    FILE *txtFile;
    uint8_t data[WIDGET_SPEAKER_TTS_FILE_MAX_SIZE] = {0};
    int32_t readLen;
    char cmdStr[WIDGET_SPEAKER_TTS_FILE_MAX_SIZE + 128];
    T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo;
    T_DjiReturnCode returnCode;

    returnCode = DjiAircraftInfo_GetBaseInfo(&aircraftInfoBaseInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("get aircraft base info error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    if (aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M3E ||
        aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M3T ||
        aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M3D ||
        aircraftInfoBaseInfo.aircraftType == DJI_AIRCRAFT_TYPE_M3TD) {
        return WidgetCziSpeaker_PlayAudioData();
    } else {
        txtFile = fopen(WIDGET_SPEAKER_TTS_FILE_NAME, "r");
        if (txtFile == NULL) {
            USER_LOG_ERROR("failed to open input file: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        readLen = fread(data, 1, WIDGET_SPEAKER_TTS_FILE_MAX_SIZE - 1, txtFile);
        if (readLen <= 0) {
            USER_LOG_ERROR("Read tts file failed, error code: %d", readLen);
            fclose(txtFile);
            return DJI_ERROR_SYSTEM_MODULE_CODE_NOT_FOUND;
        }

        data[readLen] = '\0';

        fclose(txtFile);

        USER_LOG_INFO("Read tts file success, len: %d", readLen);
        USER_LOG_INFO("Content: %s", data);

        memset(cmdStr, 0, sizeof(cmdStr));

        SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_IN_TTS_CONVERSION);

#if EKHO_INSTALLED
        /*! Attention: you can use other tts opensource function to convert txt to speech, example used ekho v7.5 */
        snprintf(cmdStr, sizeof(cmdStr), " ekho %s -s 20 -p 20 -a 100 -o %s", data,
                 WIDGET_SPEAKER_TTS_OUTPUT_FILE_NAME);
#else
        USER_LOG_WARN(
        "Ekho is not installed, please visit https://www.eguidedog.net/ekho.php to install it or use other TTS tools to convert audio");
#endif
        DjiUserUtil_RunSystemCmd(cmdStr);

        SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_PLAYING);
        USER_LOG_INFO("Start TTS Playing...");
        memset(cmdStr, 0, sizeof(cmdStr));
        snprintf(cmdStr, sizeof(cmdStr), "ffplay -nodisp -autoexit -ar 16000 -ac 1 -f s16le -i %s 2>/dev/null",
                 WIDGET_SPEAKER_TTS_OUTPUT_FILE_NAME);

        return DjiUserUtil_RunSystemCmd(cmdStr);
    }
}

static T_DjiReturnCode WidgetCziSpeaker_CheckFileMd5Sum(const char *path, uint8_t *buf, uint16_t size)
{
    MD5_CTX md5Ctx;
    uint32_t readFileTotalSize = 0;
    uint16_t readLen;
    T_DjiReturnCode returnCode;
    uint8_t readBuf[1024] = {0};
    uint8_t md5Sum[16] = {0};
    FILE *file = NULL;;

    UtilMd5_Init(&md5Ctx);

    file = fopen(path, "rb");
    if (file == NULL) {
        USER_LOG_ERROR("Open tts file error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    while (1) {
        returnCode = fseek(file, readFileTotalSize, SEEK_SET);
        if (returnCode != 0) {
            USER_LOG_INFO("fseek file error");
        }

        readLen = fread(readBuf, 1, sizeof(readBuf), file);
        // printf("MD5 read readlen:%d \n", readLen);
        if (readLen > 0) {
            readFileTotalSize += readLen;
            UtilMd5_Update(&md5Ctx, readBuf, readLen);
        }

        if (feof(file))
            break;
    }

    UtilMd5_Final(&md5Ctx, md5Sum);
    fclose(file);
    // printf("!!!!!!!!!!!!!!!!! size:%d  sizeof(md5Sum):%d \n", size, sizeof(md5Sum));
    if (size == sizeof(md5Sum)) {
        if (memcmp(md5Sum, buf, sizeof(md5Sum)) == 0) {
            USER_LOG_INFO("MD5 sum check success");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
        } else {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    } else {
        USER_LOG_ERROR("MD5 sum length error");
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static void SetSpeakerState(E_DjiWidgetSpeakerState speakerState)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
    }

    s_speakerState.state = speakerState;

    returnCode = osalHandler->MutexUnlock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
    }
}

static T_DjiReturnCode GetSpeakerState(T_DjiWidgetSpeakerState *speakerState)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    *speakerState = s_speakerState;

    returnCode = osalHandler->MutexUnlock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetWorkMode(E_DjiWidgetSpeakerWorkMode workMode)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    USER_LOG_INFO("Set widget speaker work mode: %d", workMode);
    s_speakerState.workMode = workMode;

    returnCode = osalHandler->MutexUnlock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetPlayMode(E_DjiWidgetSpeakerPlayMode playMode)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }
    
    USER_LOG_INFO("Set widget speaker play mode: %d", playMode);
    s_speakerState.playMode = playMode;

    returnCode = osalHandler->MutexUnlock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode StartPlay(void)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    s_speakerHandler->StopPlay();

    osalHandler->TaskSleepMs(5);
    USER_LOG_INFO("Start widget speaker play");
    SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_PLAYING);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode StopPlay(void)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    uint32_t pid;

    returnCode = osalHandler->MutexLock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    USER_LOG_INFO("Stop widget speaker play");
    s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_IDEL;

    pid = WidgetCziSpeaker_GetVoicePlayProcessId();
    if (pid != 0) {
        WidgetCziSpeaker_KillVoicePlayProcess(pid);
    }
    returnCode = osalHandler->MutexUnlock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetVolume(uint8_t volume)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    char cmdStr[128];
    int32_t ret = 0;
    //float realVolume;
    int realVolume;

    returnCode = osalHandler->MutexLock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    realVolume = 1.98f * (float) volume;
    s_speakerState.volume = volume;
    USER_LOG_INFO("Set widget speaker realVolume: %d", realVolume);
    USER_LOG_INFO("Set widget speaker volume: %d", volume);

    returnCode = osalHandler->MutexUnlock(s_speakerMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

// static T_DjiReturnCode ReceiveTtsData(E_DjiWidgetTransmitDataEvent event,
//                                       uint32_t offset, uint8_t *buf, uint16_t size) //接收tts数据
// {
//     uint16_t writeLen;
//     T_DjiReturnCode returnCode;

//     if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_START) {
//         USER_LOG_INFO("Create tts file.");
//         s_ttsFile = fopen(WIDGET_SPEAKER_TTS_FILE_NAME, "wb");
//         if (s_ttsFile == NULL) {
//             USER_LOG_ERROR("Open tts file error.");
//         }
//         g_cziSpeakerPlay.SpeakerSetTtsContent(event, &stream_start, 1);
//         if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
//             SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_TRANSMITTING);
//         }
//     } else if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT) {
//         USER_LOG_INFO("Transmit tts file, offset: %d, size: %d", offset, size);

//         if (s_ttsFile != NULL) {
//             fseek(s_ttsFile, offset, SEEK_SET);
//             writeLen = fwrite(buf, 1, size, s_ttsFile);
//             if (writeLen != size) {
//                 USER_LOG_ERROR("Write tts file error %d", writeLen);
//             }
//             g_cziSpeakerPlay.SpeakerSetTtsContent(event, buf, size);
//             strcpy(g_ttsContent, buf);
//         }

//         if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
//             SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_TRANSMITTING);
//         }
//     }
//     else if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH) {
//         USER_LOG_INFO("Close tts file.");
//         if (s_ttsFile != NULL) {
//             fclose(s_ttsFile);
//             s_ttsFile = NULL;
//         }

//         returnCode = WidgetCziSpeaker_CheckFileMd5Sum(WIDGET_SPEAKER_TTS_FILE_NAME, buf, size);
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("File md5 sum check failed");
//         }
//         g_cziSpeakerPlay.SpeakerSetTtsContent(event, &stream_stop, 1);
//         if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
//             SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_IDEL);
//         }
//     }

//     return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
// }

// static T_DjiReturnCode ReceiveTtsData(E_DjiWidgetTransmitDataEvent event,
//                                       uint32_t offset, uint8_t *buf, uint16_t size) //接收tts数据
// {
//     static uint32_t total_size = 0;     
//     uint16_t writeLen;
//     T_DjiReturnCode returnCode;

//     if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_START) {
//         printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
//         total_size = 0;
//     } else if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT) {
//         printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
//         USER_LOG_INFO("Transmit tts file, offset: %d, size: %d", offset, size);

//         memcpy(g_ttsContent + total_size, buf, size);
//         total_size += size;
//         printf("(%s %s LINE-%d) g_ttsContent: %s\n", __FILE__, __FUNCTION__, __LINE__, g_ttsContent);
//     }
//     else if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH) {
//         printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
//         // printf("(%s %s LINE-%d)  !!!!!!!!!!!!!!!s_ttsRunningState:%x\n", __FILE__, __FUNCTION__, __LINE__, s_ttsRunningState);

//         if (s_ttsRunningState == TTS_IDLE) {
//             // printf("!!!!!!!!!!\n");
//             g_cziSpeakerPlay.SpeakerSetTtsContent(DJI_WIDGET_TRANSMIT_DATA_EVENT_START, &stream_start, 1);
//             printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
//             g_cziSpeakerPlay.SpeakerSetTtsContent(DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT, g_ttsContent, total_size);
//             printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
//             g_cziSpeakerPlay.SpeakerSetTtsContent(DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH, &stream_stop, 1);
//             printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
//         }
//         printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);

//         // printf("\n\n*******************recv tts data from gdu******************* g_ttsContent: %s\n", g_ttsContent);
//         printf("(%s %s LINE-%d) g_ttsContent: %s\n", __FILE__, __FUNCTION__, __LINE__, g_ttsContent);
//     }

//     return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

// }

static T_DjiReturnCode ReceiveTtsData(E_DjiWidgetTransmitDataEvent event,
                                      uint32_t offset, uint8_t *buf, uint16_t size) //接收tts数据
{
    T_DjiReturnCode returnCode;

    if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_START) {
        if (s_ttsRunningState == TTS_IDLE) {
            printf("\n\n*******************start tts data rev *******************\n");
            g_cziSpeakerPlay.SpeakerSetTtsContent(DJI_WIDGET_TRANSMIT_DATA_EVENT_START, &stream_start, 1);
        }
    } else if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT) {
        if (s_ttsRunningState == TTS_IDLE) {
            printf("\n\n*******************recv tts data *******************\n");
            USER_LOG_INFO("Transmit tts file, offset: %d, size: %d", offset, size);
            g_cziSpeakerPlay.SpeakerSetTtsContent(DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT, buf, size);
        }
    }
    else if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH) {
        if (s_ttsRunningState == TTS_IDLE) {
            printf("\n\n*******************transmit tts data rev  finish*******************\n");
            char tmp_buf[1] = {'.'};

            g_cziSpeakerPlay.SpeakerSetTtsContent(DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT, tmp_buf, 1);
            g_cziSpeakerPlay.SpeakerSetTtsContent(DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH, &stream_stop, 1);
        }
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

#define AUDIO_OPUS_DECODE_FRAME_SIZE  (160)
static T_FifoBuffer gs_tFifoBufferStream = {0, 0, NULL, NULL};
static T_FifoBuffer gs_BufferStream = {0, 0, NULL, NULL};
static E_DjiWidgetTransmitDataEvent gs_evnt;
char sendBuf[230*100]={0};

static int CziSpeaker_SendStream(const char *stream, const int length, void *arg)
{
    printf("!!!!!!!!!!!!!!!!send   length:%d\n", length);
    g_cziSpeakerPlay.SpeakerSendAudioInfo(gs_evnt, stream, length);
    return 0;
}
static T_DjiReturnCode ReceiveAudioData(E_DjiWidgetTransmitDataEvent event,
                                        uint32_t offset, uint8_t *buf, uint16_t size)
{
    uint16_t writeLen;
    static uint32_t oldoffset;
    T_DjiReturnCode returnCode;
    static T_DjiWidgetTransDataContent transDataContent = {0};
    static char voiceFileName_tmp[MAXLEN_FILENAME];
    
    int len = sizeof(stream_play);
    static E_SpeakerMode s_speakermode;
    printf("state is %d, event is %d\n", s_speakerState.state, event);
    if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_START) {
        unsigned char streamplay[2] = {0x00};
        memset(streamplay, 0x00, 2);
        streamplay[0] = stream_play;
        streamplay[1] = 0x02;
        
        s_isDecodeFinished = false;
        memcpy(&transDataContent, buf, size);
        s_decodeBitrate = transDataContent.transDataStartContent.fileDecodeBitrate;

        USER_LOG_INFO("Create voice file: %s, decoder bitrate: %d.", transDataContent.transDataStartContent.fileName,
                      transDataContent.transDataStartContent.fileDecodeBitrate);
        /* 保存文件名 */
        memset(gs_voiceFilename,  '\0', MAXLEN_FILENAME);
        memset(voiceFileName_tmp, '\0', MAXLEN_FILENAME);
        strcpy(voiceFileName_tmp, transDataContent.transDataStartContent.fileName);
        snprintf(gs_voiceFilename, MAXLEN_FILENAME, "%s", voiceFileName_tmp);

        int offset = strlen(gs_voiceFilename);
                // audio is tts, set tag in file
        if(s_speakerState.workMode == DJI_WIDGET_SPEAKER_WORK_MODE_VOICE && transDataContent.transDataStartContent.fileDecodeBitrate == WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_FOR_REAL_RECORD){
            s_speakermode = IS_RECORD;
            snprintf((char *)&gs_voiceFilename[offset], MAXLEN_FILENAME - offset, "%s", ".record");
            USER_LOG_INFO("IS_RECORD");
            FifoBuffer_Init(&gs_tFifoBufferStream, AUDIO_OPUS_DECODE_FRAME_SIZE);
        }else if(s_speakerState.workMode == DJI_WIDGET_SPEAKER_WORK_MODE_VOICE && transDataContent.transDataStartContent.fileDecodeBitrate == WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_FOR_MUSIC){
            s_speakermode = IS_UPLOAD_RECORD;
            snprintf((char *)&gs_voiceFilename[offset], MAXLEN_FILENAME - offset, "%s", ".opus");
            SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_PLAYING);
            USER_LOG_INFO("IS_UPLOAD_RECORD");
        }else if(s_speakerState.workMode == DJI_WIDGET_SPEAKER_WORK_MODE_TTS && transDataContent.transDataStartContent.fileDecodeBitrate == WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_FOR_MUSIC){
            s_speakermode = IS_TTS_AUDIO;
            snprintf((char *)&gs_voiceFilename[offset], MAXLEN_FILENAME - offset, "%s", ".tts");
            USER_LOG_INFO("IS_TTS_AUDIO");
        }
        g_cziSpeakerPlay.SpeakerSetPlayInfo(s_speakermode, gs_voiceFilename);

        memset(voiceAbsolutePath, '\0', MAXLEN_FILEPATH);
        snprintf(voiceAbsolutePath, MAXLEN_FILEPATH, "%s/%s", FILEPATH_MUSIC_FILE, gs_voiceFilename);

        USER_LOG_INFO("Create voice file.");
        s_audioFile = fopen(voiceAbsolutePath, "wb+");
        if (s_audioFile == NULL) {
            USER_LOG_ERROR("Create tts file error.");
        }
        if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
            SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_TRANSMITTING);
        }

        g_cziSpeakerPlay.SpeakerSendAudioInfo(event, streamplay, 2);   

    } else if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT) {


        USER_LOG_INFO("Transmit voice file, offset: %d, size: %d", offset, size);  // 970  230    1200
        

        if (s_audioFile != NULL) {

            // fseek(s_audioFile, offset, SEEK_SET);
            // writeLen = fwrite(buf, 1, size, s_audioFile);

            if (errOffset) {
                int sLen = offset % AUDIO_OPUS_DECODE_FRAME_SIZE;
                int bufOffset = 0;
                uint8_t *p = buf;
                // int curSize = size;
                if (sLen > 0) {
                    printf("(%s %s LINE-%d)init voice, offset: %d, curSize: %d, sLen: %d\n", __FILE__, __FUNCTION__, __LINE__, offset, size, sLen);
                    bufOffset = size - sLen;
                    p = &buf[bufOffset];
                    // curSize = sLen;
                    printf("(%s %s LINE-%d)init voice, offset: %d, errOffset: %d, sLen: %d, bufOffset: %d\n",__FILE__, __FUNCTION__, __LINE__, offset, errOffset, sLen, bufOffset);
                }

                // if (sLen > 0) {
                    g_cziSpeakerPlay.SpeakerSendAudioInfo(event, p, size);
                // }
                printf("init voice, errOffset: %d, sLen: %d, bufOffset: %d\n", errOffset, sLen, bufOffset);
                errOffset = 0;
                
            }
            else {
                
                g_cziSpeakerPlay.SpeakerSendAudioInfo(event, buf, size);
                // if (writeLen != size) {
                //     USER_LOG_ERROR("Write tts file error %d", writeLen);
                //     g_cziSpeakerPlay.SpeakerSendAudioInfo(event, buf, size);
                // }

                fseek(s_audioFile, offset, SEEK_SET);
                writeLen = fwrite(buf, 1, size, s_audioFile);
                // g_cziSpeakerPlay.SpeakerSendAudioInfo(event, buf, size);
                if (writeLen != size) {
                USER_LOG_ERROR("Write tts file error %d", writeLen);
            }
            }


        }



        // USER_LOG_INFO("Transmit voice file, offset: %d, size: %d", offset, size);
        // if (s_audioFile != NULL) {
        //     fseek(s_audioFile, offset, SEEK_SET);
        //     writeLen = fwrite(buf, 1, size, s_audioFile);
        //     g_cziSpeakerPlay.SpeakerSendAudioInfo(event, buf, size);
        //     if (writeLen != size) {
        //         USER_LOG_ERROR("Write tts file error %d", writeLen);
        //     }
        // }

        if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
            SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_TRANSMITTING);
        }
        
    } else if (event == DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH && s_audioFile != NULL) {
        g_cziSpeakerPlay.SpeakerSendAudioInfo(event, &stream_stop, 1);
        USER_LOG_INFO("Close voice file.");
        fclose(s_audioFile);
        s_audioFile = NULL;

        returnCode = WidgetCziSpeaker_CheckFileMd5Sum(voiceAbsolutePath, buf, size);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("File md5 sum check failed");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
        // if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
        //     SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_IDEL);
        // }
        if (s_speakermode == IS_RECORD) {
            if (FifoBuffer_HandleLastData(&gs_tFifoBufferStream, NULL, CziSpeaker_SendStream)) {
            // CziLog_Error(" FifoBuffer_Handle opus failed\n", __FUNCTION__, __LINE__);
                USER_LOG_ERROR("FifoBuffer_Handle DecoderOpus_Decode failed\n" );
            }
        
            FifoBuffer_Exit(&gs_tFifoBufferStream);
        }

        
        // WidgetCziSpeaker_DecodeAudioData();
    }
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

#ifndef __CC_ARM
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif
// int WidgetCziSpeaker_GetSpeakerStateWork(int statework) 
// {
//     statework = s_speakerState.workMode;
//     return 0;
// }


static void *WidgetCziSpeaker_WidgetSpeakerTask(void *arg)
{
    T_DjiReturnCode djiReturnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    USER_UTIL_UNUSED(arg);

    while (1) {
        osalHandler->TaskSleepMs(10);

        if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
            osalHandler->TaskSleepMs(500);
            continue;
        }
        if (s_speakerState.playMode == DJI_WIDGET_SPEAKER_PLAY_MODE_LOOP_PLAYBACK) {
            if (s_speakerState.workMode == DJI_WIDGET_SPEAKER_WORK_MODE_VOICE) {
                USER_LOG_DEBUG("Waiting opus decoder finished...");
                while (s_isDecodeFinished == false) {
                    osalHandler->TaskSleepMs(1);
                }
                // djiReturnCode = WidgetCziSpeaker_PlayAudioData();
                // if (djiReturnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                //     USER_LOG_ERROR("Play audio data failed, error: 0x%08llX.", djiReturnCode);
                // }
            } 
            else {
                // djiReturnCode = WidgetCziSpeaker_PlayTtsData();
                // if (djiReturnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                //     USER_LOG_ERROR("Play tts data failed, error: 0x%08llX.", djiReturnCode);
                // }
            }
            osalHandler->TaskSleepMs(1000);
        } 
        else {
            if (s_speakerState.workMode == DJI_WIDGET_SPEAKER_WORK_MODE_VOICE) {
                USER_LOG_DEBUG("Waiting opus decoder finished...");
                // djiReturnCode = WidgetCziSpeaker_PlayAudioData();
                // if (djiReturnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                //     USER_LOG_ERROR("Play audio data failed, error: 0x%08llX.", djiReturnCode);
                // }
            } else {
                // djiReturnCode = WidgetCziSpeaker_PlayTtsData();
                // if (djiReturnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                //     USER_LOG_ERROR("Play tts data failed, error: 0x%08llX.", djiReturnCode);
                // }
            }

            djiReturnCode = osalHandler->MutexLock(s_speakerMutex);
            if (djiReturnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("lock mutex error: 0x%08llX.", djiReturnCode);
            }

            if (s_speakerState.playMode == DJI_WIDGET_SPEAKER_PLAY_MODE_SINGLE_PLAY) {
                // s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_IDEL;
            }

            djiReturnCode = osalHandler->MutexUnlock(s_speakerMutex);
            if (djiReturnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("unlock mutex error: 0x%08llX.", djiReturnCode);
            }
        }
    }
}


T_DjiReturnCode WidgetCziSpeaker_UpdateWorkMode(E_DjiWidgetSpeakerWorkMode workMode)
{
    s_speakerState.workMode = workMode;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode WidgetCziSpeaker_UpdateTtsRunningState(E_TtsRunningState ttsRunningState)
{
    s_ttsRunningState = ttsRunningState;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

E_TtsRunningState WidgetCziSpeaker_GetTtsRunningState(void)
{
    return s_ttsRunningState;
}


int WidgetCziSpeaker_GetMediaMode(int mode) 
{
    MediaMode = mode;
    return 0;
}




int WidgetCziSpeaker_GetErro(uint32_t erro)
{
    errOffset = erro;
    // sendFailedFlag = flag;
    // printf("!!!!!!!!send :%d   %d\n", sendFailedFlag, errorNum);
    printf("!!!!!!!!send errOffset:%d\n", errOffset);
    return 0;
}

#ifndef __CC_ARM
#pragma GCC diagnostic pop
#endif

/****************** (C) COPYRIGHT DJI Innovations *****END OF FILE****/