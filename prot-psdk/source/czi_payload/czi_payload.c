#include "dji_logger.h"
#include "dji_widget.h"
#include "widget/widget.h"
#include "widget_czi_speaker/widget_czi_speaker.h"

#include "czi_transmission/czi_transmission.h"
#include "czi_megaphone/czi_megaphone.h"
#include "czi_msdk_handler/czi_msdk_handler.h"
#include "czi_psdk_handler/czi_psdk_handler.h"
#include "czi_light/czi_light.h"
#include "czi_json_handler/czi_json_handler.h"
#include "../dji_module/fc_subscription/test_fc_subscription.h"

#include "czi_payload.h"
#include "czi_log.h"
#include "../psdk_config.h"
#include "czi_video/czi_video.h"
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/inotify.h>
#include "czi_gimbal/czi_gimbal.h"
// #include "czi_gimbal.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

typedef struct _WidgetHandler
{
    E_WidgetIndex eWidgetIndex;
    int (*widgetHandle)(void *arg, const int len);
} T_WidgetHandler, *PT_WidgetHandler;

typedef struct _MusicCtrlHandler
{
    E_MusicCtrlIndex eMusicCtrlIndex;
    int (*musicCtrlHandle)(void);
} T_MusicCtrlHandler, *PT_MusicCtrlHandler;

static T_MusicListInfo gs_musicListInfo;
static T_GimbalLightInfo gs_gimbalLightInfo;
static T_MegaphoneInfo gs_megaphoneInfo;

static pthread_t videoThread;
static bool videoThreadRunning = false;
static bool isStreaming = false;
static int breathLight = 1;
static int ttsVoice = 0;
static int ttsSpeed = 0;
static T_DjiWidgetSpeakerState s_payloadSpeakerState = {0};  //widget  speaker state

T_WidgetCziMediaInfo WidgetCziMediaInfo;

static T_DjiReturnCode Payload_HandleWidgetAction(E_DjiWidgetType widgetType, uint32_t index, int32_t value);
static int Payload_ControlMusic(void *arg, const int len);
static int Payload_ControlLight(void *arg, const int len);
static int Payload_BreathingLinght(void *arg, const int len);
static int Payload_SetMediaVolume(void *arg, const int len);
static int Payload_SetTtsVoice(void *arg, const int len);
static int Payload_SetTtsSpeed(void *arg, const int len);
static int Payload_SetMediaMode(void *arg, const int len);
static int Payload_EnableVersionBroadcast(void *arg, const int len);
// static int Payload_SelectMusicDir(void *arg, const int len);
static int Payload_DeleteMusic(void *arg, const int len);
static int Payload_SwitchMode(void *arg, const int len);
static int Payload_VideoStream(void *arg, const int len);

static int Payload_CtrlMusicLast(void);
static int Payload_CtrlMusicNext(void);
static int Payload_CtrlMusicPlay(void);
static int Payload_CtrlMusicStop(void);
static void *videoStreamThread(void *arg);

T_DjiReturnCode CziPayload_SendMediaInfoToWidget(void);

T_DjiReturnCode CziPayload_ApplicationStart(void)
{
    T_DjiReturnCode returnCode;

    CziTransmission_Init();

    CziMsdkHandler_Init();

    CziPsdkHandler_Init();

    // CziVideo_Init();
    /* enable widget */
    returnCode = DjiTest_WidgetStartService(Payload_HandleWidgetAction);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS)
    {
        USER_LOG_ERROR("enable widget service error");

        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // CziWidget_SetWidgetValue(WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE);

    /* enable megaphone */
    T_DjiWidgetSpeakerHandler speakerCallbackList = {
        .GetSpeakerState = CziMegaphone_GetSpeakerState,
        .StopPlay = CziMegaphone_StopPlay,
        .SetPlayMode = CziMegaphone_SetPlayMode,
        .SetVolume = CziMegaphone_SetVolume,
        .StartPlay = CziMegaphone_StartPlay,
        .SetWorkMode = CziMegaphone_SetWorkMode,
    };
    returnCode = WidgetCziSpeaker_WidgetSpeakerStartService(&speakerCallbackList);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS)
    {
        USER_LOG_ERROR("enable speaker widget error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    T_CziSpeakerPlay cziSpeakerPlay = {
        .SpeakerSetTtsContent = CziMegaphone_SetTtsInfo,
        .SpeakerSetPlayInfo = CziMegaphone_SetPlayInfo,
        .SpeakerSendAudioInfo = CziMegaphone_SendAudioInfo,
    };
    returnCode = WidgetCziSpeaker_RegistPlay(&cziSpeakerPlay);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS)
    {
        USER_LOG_ERROR("WidgetCziSpeaker_RegistPlay error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
    gs_musicListInfo.mediastats = 0;
    // gs_musicListInfo.eDirType = DIR_TYPE_MUSIC;
    CziMegaphone_GetMusicSum(&gs_musicListInfo);
    CziMegaphone_GetMusicName(&gs_musicListInfo, 1);


    CziJsonHandler_Open(FILEPATH_JSON_CONFIG_USR, 0);
    CziJsonHandler_ReadInt(JSON_KEY_BREATH_LIGHT, &breathLight);
    CziJsonHandler_Close(1);
    char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
    DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_SWITCH, WIDGET_INDEX_BREATHING_LIGHT, breathLight, &widgetCziServiceCmd);



    CziJsonHandler_Open(FILEPATH_JSON_STATE_TTS, 0);
    CziJsonHandler_ReadInt(JSON_KEY_TTS_VOICE, &ttsVoice);
    CziJsonHandler_ReadInt(JSON_KEY_TTS_SPEED, &ttsSpeed);
    CziJsonHandler_Close(1);

    CziMegaphone_SetTtsVoice(ttsVoice);
    CziMegaphone_SetTtsSpeed(ttsSpeed);

    DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_SCALE, WIDGET_INDEX_TTS_VOICE, ttsVoice, &widgetCziServiceCmd);
    DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_SCALE, WIDGET_INDEX_TTS_SPEED, ttsSpeed, &widgetCziServiceCmd);
    
    CziPayload_SendMediaInfoToWidget();

    // CziJsonHandler_Open(FILEPATH_JSON_STATE_PSDK, 0);
    // CziJsonHandler_ReadInt(JSON_KEY_ANGLE_TUNING,&gs_GimbalTuning);

    // CziJsonHandler_Close(1);
    // // CziGimbal_GetGimbalTuning(gs_GimbalTuning);

    // char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
    // DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_SCALE, WIDGET_INDEX_GIMBAL_ANGLE, 50+gs_GimbalTuning, &widgetCziServiceCmd);
    
    CziMegaphone_GetBasicInfo();

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode CziPayload_SendMediaInfoToWidget(void)
{
    T_DjiReturnCode returnCode;

    WidgetCziMediaInfo.musicListIndex = gs_musicListInfo.musicListIndex,
    memset(WidgetCziMediaInfo.musicName, 0x00, sizeof(gs_musicListInfo.musicName));
    memcpy(WidgetCziMediaInfo.musicName, gs_musicListInfo.musicName, sizeof(gs_musicListInfo.musicName));
    WidgetCziMediaInfo.sum = gs_musicListInfo.sum;

    WidgetCziMediaInfo.mediaMode = gs_megaphoneInfo.mediaMode;
    WidgetCziMediaInfo.mediaStats = gs_megaphoneInfo.mediaStats;
    WidgetCziMediaInfo.volume = gs_megaphoneInfo.volume;

    WidgetCziMediaInfo.aLightSwitch = gs_gimbalLightInfo.alightSwitch;

    returnCode = Widget_MediaListInfo(&WidgetCziMediaInfo);

    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS)
    {
        USER_LOG_ERROR("WidgetCziSpeaker_RegistPlay error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;     
}

void CziPayload_NormalExitHandler(int signalNum)
{
    CziVideo_Deinit();
}

static T_DjiReturnCode Payload_HandleWidgetAction(E_DjiWidgetType widgetType, uint32_t index, int32_t value)
{
    printf("(%s %s LINE-%d)  (widget index: %d) (value: %d)   \n", __FILE__, __FUNCTION__, __LINE__, index, value);
    static T_WidgetHandler s_tWidgetHandler[] = {
        {WIDGET_INDEX_MUSIC_CONTROL, Payload_ControlMusic},

        
        {WIDGET_INDEX_FLASH_SWITCH, Payload_ControlLight},
        // {WIDGET_INDEX_ALARM_BLOW, Payload_ControlBlow},
        {WIDGET_INDEX_MODULE_SWITCH, Payload_SwitchMode},
        // {WIDGET_INDEX_GIMBAL_ANGLE, Payload_ControlGimbalAngle},
        // { WIDGET_INDEX_LIGHT_SPOT_AI, Payload_VideoStream },

        {WIDGET_INDEX_VOLUME, Payload_SetMediaVolume},
        {WIDGET_INDEX_TTS_VOICE, Payload_SetTtsVoice},
        {WIDGET_INDEX_TTS_SPEED, Payload_SetTtsSpeed},
        {WIDGET_INDEX_MUSIC_SINGLE_CYCLE, Payload_SetMediaMode},
        // { WIDGET_INDEX_MUSIC_DIR_SELECT, Payload_SelectMusicDir },
        {WIDGET_INDEX_DELETE_MUSIC, Payload_DeleteMusic},
        { WIDGET_INDEX_BREATHING_LIGHT, Payload_BreathingLinght},
        {WIDGET_INDEX_VERSION_BROADCAST, Payload_EnableVersionBroadcast},
        // { WIDGET_INDEX_LOCKGIMBAL, NULL },
        // { WIDGET_INDEX_LEFTPITCHADJ, NULL },
        // { WIDGET_INDEX_RIGHTPITCHADJ, NULL },
        // { WIDGET_INDEX_SAVEADJ, NULL },
    };

    int count = sizeof(s_tWidgetHandler) / sizeof(s_tWidgetHandler[0]);
    for (int idx = 0; idx < count; idx++)
    {
        if (index != s_tWidgetHandler[idx].eWidgetIndex)
            continue;
        if (s_tWidgetHandler[idx].widgetHandle == NULL)
        {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
        char data = (char)value;
        int len = 1;
        int ret = s_tWidgetHandler[idx].widgetHandle(&data, len);
        break;
    }
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

/* light */
static int Payload_ControlLight(void *arg, const int len)
{
    char lightype = *(char *)arg;
    gs_gimbalLightInfo.alightSwitch = lightype;
    printf("(%s %s LINE-%d) lightype:%x\n", __FILE__, __FUNCTION__, __LINE__, lightype);
    CziLight_ControlLight(lightype);
    CziPayload_SendMediaInfoToWidget();
    return 0;
}

static int Payload_BreathingLinght(void *arg, const int len)
{
    char breathlight = *(char *)arg;
    gs_gimbalLightInfo.breathLight = breathlight;
    printf("(%s %s LINE-%d) lightype:%x\n", __FILE__, __FUNCTION__, __LINE__, breathlight);
    // CziLight_BreathingLight(breathlight);
    CziMegaphone_SetBreathLight(breathlight);
    

    return 0;
}


static int Payload_CtrlMusicLast(void)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    if (gs_musicListInfo.sum == 0)
    {
        char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
        DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_LIST, WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE, &widgetCziServiceCmd);
        return 0; /*如果没有音乐文件，不执行操作*/
    }

    if ((gs_musicListInfo.musicListIndex - 1) == 0)
    {
        gs_musicListInfo.musicListIndex = gs_musicListInfo.sum;
    }
    else
    {
        gs_musicListInfo.musicListIndex = gs_musicListInfo.musicListIndex - 1;
    }

    CziMegaphone_GetMusicName(&gs_musicListInfo, gs_musicListInfo.musicListIndex);

    if (gs_musicListInfo.mediastats)
    {
        CziMegaphone_CtrlMusicStop();
        // osalHandler->TaskSleepMs(1000);
        CziMegaphone_CtrlMusicPlay(gs_musicListInfo);
    }
    CziPayload_SendMediaInfoToWidget();
    char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
    DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_LIST, WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE, &widgetCziServiceCmd);
    return 0;
}

static int Payload_CtrlMusicNext(void)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    if (gs_musicListInfo.sum == 0)
    {
        char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
        DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_LIST, WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE, &widgetCziServiceCmd);
        return 0; // 如果没有音乐文件，不执行操作
    }

    if ((gs_musicListInfo.musicListIndex + 1) > gs_musicListInfo.sum)
    {
        gs_musicListInfo.musicListIndex = 1;
    }
    else
    {
        gs_musicListInfo.musicListIndex = gs_musicListInfo.musicListIndex + 1;
    }

    CziMegaphone_GetMusicName(&gs_musicListInfo, gs_musicListInfo.musicListIndex);
    printf("gs_megaphoneInfo.mediaStats is %d\n", gs_megaphoneInfo.mediaStats);
    if (gs_musicListInfo.mediastats)
    {
        CziMegaphone_CtrlMusicStop();
        // osalHandler->TaskSleepMs(1000);
        CziMegaphone_CtrlMusicPlay(gs_musicListInfo);
    }

    CziPayload_SendMediaInfoToWidget();
    char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
    DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_LIST, WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE, &widgetCziServiceCmd);
    return 0;
}

static int Payload_CtrlMusicPlay(void)
{
    CziMegaphone_CtrlMusicPlay(gs_musicListInfo);
    gs_musicListInfo.mediastats = 1;
    char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
    DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_LIST, WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE, &widgetCziServiceCmd);
    
    CziMegaphone_GetSpeakerState(&s_payloadSpeakerState); //get Speakerstate
    gs_megaphoneInfo.volume = s_payloadSpeakerState.volume;
    gs_megaphoneInfo.mediaMode = s_payloadSpeakerState.playMode;
    if (s_payloadSpeakerState.state == DJI_WIDGET_SPEAKER_STATE_PLAYING)
        gs_megaphoneInfo.mediaStats = 1;
    CziPayload_SendMediaInfoToWidget();
    
    return 0;
}

static int Payload_CtrlMusicStop(void)
{
    // CziWidget_SetWidgetValue(WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE);
    CziMegaphone_CtrlMusicStop();
    unsigned char Amp = 0x41;
    CziWidget_GetRecordAmp(Amp);
    gs_musicListInfo.mediastats = 0;
    char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
    DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_LIST, WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE, &widgetCziServiceCmd);

    //////////////////////////
    CziMegaphone_GetSpeakerState(&s_payloadSpeakerState); //get Speakerstate
    gs_megaphoneInfo.mediaMode = s_payloadSpeakerState.playMode;

    CziPayload_SendMediaInfoToWidget();
    return 0;
}

/* megaphone */
static int Payload_ControlMusic(void *arg, const int len)
{
    static T_MusicCtrlHandler s_tMusicCtrlHandler[] = {
        {MUSIC_CTRL_INDEX_LAST_SONG, Payload_CtrlMusicLast},
        {MUSIC_CTRL_INDEX_NEXT_SONG, Payload_CtrlMusicNext},
        {MUSIC_CTRL_INDEX_PLAY_SONG, Payload_CtrlMusicPlay},
        {MUSIC_CTRL_INDEX_STOP_SONG, Payload_CtrlMusicStop},
    };

    char musicCtrlIdx = *(char *)arg;
    int count = sizeof(s_tMusicCtrlHandler) / sizeof(s_tMusicCtrlHandler[0]);
    for (int idx = 0; idx < count; idx++)
    {
        if (musicCtrlIdx != s_tMusicCtrlHandler[idx].eMusicCtrlIndex)
            continue;
        if (s_tMusicCtrlHandler[idx].musicCtrlHandle == NULL)
        {
            break;
        }
        int ret = s_tMusicCtrlHandler[idx].musicCtrlHandle();
        if (ret)
        {
        }
        break;
    }

    // CziWidget_SetWidgetValue(WIDGET_INDEX_MUSIC_CONTROL, MUSIC_CTRL_INDEX_CTRL_IDLE);

    return 0;
}



static int Payload_SetMediaVolume(void *arg, const int len)
{
    char scaleVolume = *(char *)arg;
    printf("(%s %s LINE-%d) volume:%x\n", __FILE__, __FUNCTION__, __LINE__, scaleVolume);
    CziMegaphone_SetVolume(scaleVolume);
    return 0;
}

static int Payload_SetTtsVoice(void *arg, const int len)
{
    ttsVoice = *(char *)arg;
    printf("!!!!!!!!!!!!!!!!ttsVoice = %x\n", ttsVoice);
    CziMegaphone_SetTtsVoice(ttsVoice);
    
    
    return 0;
}

static int Payload_SetTtsSpeed(void *arg, const int len)
{
    ttsSpeed = *(char *)arg;
    CziMegaphone_SetTtsSpeed(ttsSpeed);

    return 0;
}

static int Payload_SetMediaMode(void *arg, const int len)
{
    char playMode = *(char *)arg;
    printf("111111111111111111111");
    printf("(%s %s LINE-%d) playMode:%x\n", __FILE__, __FUNCTION__, __LINE__, playMode);
    CziMegaphone_SetPlayMode(playMode);
    gs_megaphoneInfo.mediaMode = playMode;
    return 0;
}

static int Payload_EnableVersionBroadcast(void *arg, const int len)
{
    char Bdvalue = *(char *)arg;
    printf("(%s %s LINE-%d) Bdvalue:%x\n", __FILE__, __FUNCTION__, __LINE__, Bdvalue);
    CziMegaphone_StartBroadcast(Bdvalue);
    return 0;
}

// static int Payload_SelectMusicDir(void *arg, const int len)
// {
//     char DirList = *(char *)arg;

//     gs_musicListInfo.eDirType = DirList;
//     CziMegaphone_GetMusicSum(&gs_musicListInfo);
//     if (gs_musicListInfo.sum > 0) {
//         CziMegaphone_GetMusicName(&gs_musicListInfo, 1);
//     } else {
//         memset(gs_musicListInfo.musicName, 0, sizeof(gs_musicListInfo.musicName));
//         gs_musicListInfo.musicListIndex = 0;
//     }
//     CziPayload_SendMediaInfoToWidget();

//     return 0;
// }

static int Payload_DeleteMusic(void *arg, const int len)
{
    char value = *(char *)arg;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    if (value)
    {
        int index = gs_musicListInfo.musicListIndex;
        CziMegaphone_CtrlMusicStop();
        // osalHandler->TaskSleepMs(200);
        if (gs_musicListInfo.sum == 0)
            return 0;
        gs_musicListInfo.sum--;
        CziMegaphone_DeleteMusic(0x00, index);
        // osalHandler->TaskSleepMs(500);
        if (gs_musicListInfo.sum == 0)
        {
            gs_musicListInfo.musicListIndex = 0;
            memset(gs_musicListInfo.musicName, 0, sizeof(gs_musicListInfo.musicName));
        }
        else
        {
            if (index > gs_musicListInfo.sum)
            {
                gs_musicListInfo.musicListIndex = gs_musicListInfo.sum;
            }
            CziMegaphone_GetMusicSum(&gs_musicListInfo);
            CziMegaphone_GetMusicName(&gs_musicListInfo, gs_musicListInfo.musicListIndex);
        }
        CziPayload_SendMediaInfoToWidget();
    }
    return 0;
}

static int Payload_SwitchMode(void *arg, const int len)
{
    char modulemode = *(char *)arg;
    CziMegaphone_SetMediaMode(modulemode);
}


//////////2
// static int Payload_VideoStream(void *arg, const int len)
// {
//     int videoStreamFlag;
//     CziWidget_GetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, &videoStreamFlag);
//     CziMegaphone_SetAiVideoState(videoStreamFlag);
//     return 0;
// }

//CZI & DJI  状态同步
//psdk diaoy 1
int Payload_SetMediaInfo(int stats)
{
    gs_musicListInfo.mediastats = stats;
    return 0;
}
/////////2

// static int gs_GimbalTuning = 0;
// static int gs_GimbalFunTine_old = 50;



int CziPayload_SetGimbalAngle(int angle)
{
    // int zero = 0;
    gs_gimbalLightInfo.gLightAngle = angle;
    CziGimbal_ControlGimbalAngle(angle);
    return 0;
}

int CziPayload_BalancePower(int BatteryValue)
{
    float totalPower = BatteryValue * 0.001 * TOTAL_CURRENT; //get Power

    CziMegaphone_GetSpeakerState(&s_payloadSpeakerState); //get Speakerstate
    gs_megaphoneInfo.volume = s_payloadSpeakerState.volume;
    gs_megaphoneInfo.mediaMode = s_payloadSpeakerState.playMode;
    // if (s_payloadSpeakerState.state == DJI_WIDGET_SPEAKER_STATE_PLAYING)
    //     gs_megaphoneInfo.mediaStats = 1;
    // USER_LOG_INFO("totalPower is (%f), gs_megaphoneInfo.volume is (%d), gs_megaphoneInfo.mediaStats is (%d)!", totalPower, gs_megaphoneInfo.volume, gs_megaphoneInfo.mediaStats);
    CziPayload_SendMediaInfoToWidget();

    if(totalPower < LOWPOWER_LIMIT) {
        if (gs_gimbalLightInfo.gLightSwitch == SWITCH_OPEN && gs_megaphoneInfo.mediaStats == 1) {
            if (gs_megaphoneInfo.volume == 100 && gs_gimbalLightInfo.gLightBright > 30) {
                // CziGimbal_ControlGimbalBright(30);
                // gs_gimbalLightInfo.gLightBright = 30;
            }
            else if (gs_gimbalLightInfo.gLightBright == 100 && gs_megaphoneInfo.volume > 90) {
                CziMegaphone_SetVolume(90);
                gs_megaphoneInfo.volume = 90;
            }
        }
    }else if (totalPower < HIGHPOWER_LIMIT){
        if (gs_gimbalLightInfo.gLightSwitch == SWITCH_OPEN && gs_megaphoneInfo.mediaStats == 1) {
            if (gs_megaphoneInfo.volume == 100 && gs_gimbalLightInfo.gLightBright > 90) {
                // CziGimbal_ControlGimbalBright(90);
                // gs_gimbalLightInfo.gLightBright = 90;
            }
            else if (gs_gimbalLightInfo.gLightBright == 100 && gs_megaphoneInfo.volume > 90) {
                CziMegaphone_SetVolume(90);
                gs_megaphoneInfo.volume = 90;
            }
        }
    }

    return 0;
}

//add
int CziPayload_UpdateMediaWidget(void)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    // osalHandler->TaskSleepMs(500);
    CziMegaphone_GetMusicSum(&gs_musicListInfo);
    CziMegaphone_GetSpeakerState(&s_payloadSpeakerState); //get Speakerstate

    gs_megaphoneInfo.volume = s_payloadSpeakerState.volume;
    gs_megaphoneInfo.mediaMode = s_payloadSpeakerState.playMode;
    if (s_payloadSpeakerState.state == DJI_WIDGET_SPEAKER_STATE_PLAYING)
        gs_megaphoneInfo.mediaStats = 1;
    else 
        gs_megaphoneInfo.mediaStats = 0;

    CziPayload_SendMediaInfoToWidget();
    return 0;
}
#define ARM_ANGLE_MAX         90
#define ARM_ANGLE_MIN         0




T_megaphone mega;
int oldAngle = 0;
int mega_setAngle(int angle)
{
    int newAngle = angle;
    // char cmdStr[256] = {0};

    // if( a<ARM_ANGLE_MIN )
    // {
    //     logWarn("Warn: angle = %d, reset to %d degree", a, ARM_ANGLE_MIN);
    //     a = ARM_ANGLE_MIN;
    // }
    // else if( a>ARM_ANGLE_MAX )
    // {
    //     logWarn("Warn: angle = %d, reset to %d degree", a, ARM_ANGLE_MAX);
    //     a = ARM_ANGLE_MAX;
    // }

    if(newAngle > ARM_ANGLE_MAX)
        newAngle =ARM_ANGLE_MAX;
    else if(newAngle < ARM_ANGLE_MIN)
        newAngle = ARM_ANGLE_MIN;
    else if(newAngle != oldAngle)
        oldAngle = newAngle;
    else
    {
    //    logInfo("  set same angle = %d", a);
        return 0;
    }

    // checkPsdkUsing();
    // snprintf(cmdStr, sizeof(cmdStr), "%s %d", SHELL_SERVO, a);
    // logInfo("  set angle = %d", a);
    // return runSystemCmd(cmdStr);
    return 0;
}


/////////////////////
// static int gs_GimbalTuning = 0;
// static int gs_GimbalFunTine_old = 50;

// int Payload_ControlGimbalAngle(void *arg, const int len)
// {
//     char angle = *(char *)arg;

//     gs_GimbalTuning = angle - gs_GimbalFunTine_old;
//     printf("angle: %d, gs_GimbalTuning is %d\n", angle, gs_GimbalTuning);
//     CziGimbal_ControlGimbalAngle(gs_GimbalTuning, gs_gimbalLightInfo.gLightAngle);
    
//     char widgetCziServiceCmd = WIDGET_CZI_SERVICE_CMD_IGNORE;
//     DjiTestWidget_SetWidgetValue(DJI_WIDGET_TYPE_SCALE, WIDGET_INDEX_GIMBAL_ANGLE, angle, &widgetCziServiceCmd);
//     return 0;
// }

