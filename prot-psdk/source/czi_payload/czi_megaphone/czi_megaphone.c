#include "../../dji_module/widget/widget.h"
#include "../../dji_module/widget_czi_speaker/widget_czi_speaker.h"
#include "dji_logger.h"
#include "../czi_payload.h"
#include "../czi_transmission/czi_transmission.h"
#include "../czi_protocol_handler/protocol_longFormatHandler.h"
#include "../czi_protocol_handler/protocol_commonHandler.h"
#include "../czi_json_handler/czi_json_handler.h"
#include "../../psdk_config.h"
#include "media_handler/media_handler.h"
#include "czi_megaphone.h"
#include "dirent.h"
#include <sys/stat.h>




static int CziMegaphone_TtsTypeAdjust(char ttsVoice);

E_MediaMode gMediaMode = RECORD_MODE;
static char gs_RecordFilename[MAXLEN_FILE_NAME];
E_SpeakerMode g_speakerMode;
static T_DjiMutexHandle s_speakerMutex = {0};
static T_DjiWidgetSpeakerState s_speakerState = {0};
static int s_DecodeBitrate = 0;
E_TtsMode g_TtsMode = TTS_STREAM_MODE;
char ttsVoice;
int statework;



int ttstype[38] = {VOICE_TYPE_XIAOYAN, VOICE_TYPE_XIAOFENG, VOICE_TYPE_FE_BDL, VOICE_TYPE_FE_SLT, VOICE_TYPE_XIAOQIAN, VOICE_TYPE_XIAOMEI,\
    VOICE_TYPE_XIAOQIANG, VOICE_TYPE_XIAOKUN, VOICE_TYPE_XIAORONG, VOICE_TYPE_XIAOYING, VOICE_TYPE_DE, VOICE_TYPE_EN_029, VOICE_TYPE_EN_GB, \
    VOICE_TYPE_EN_GB_SCOTLAND, VOICE_TYPE_EN_GB_X_GBCLAN, VOICE_TYPE_EN_GB_X_GBCWMD, VOICE_TYPE_EN_GB_X_RP, VOICE_TYPE_EN_US, VOICE_TYPE_FR_BE, \
    VOICE_TYPE_FR_CH, VOICE_TYPE_FR_FR, VOICE_TYPE_IT, VOICE_TYPE_JA, VOICE_TYPE_RU, VOICE_TYPE_RU_LV, VOICE_TYPE_ES, VOICE_TYPE_ES_419, \
    VOICE_TYPE_AR, VOICE_TYPE_PT, VOICE_TYPE_PT_BR, VOICE_TYPE_PL, VOICE_TYPE_KO, VOICE_TYPE_MY, VOICE_TYPE_NE, VOICE_TYPE_VI, VOICE_TYPE_JAPANESE, \
    VOICE_TYPE_AHW, VOICE_TYPE_SLT};
// int ttstype[38] = { VOICE_TYPE_XIAOFENG, VOICE_TYPE_FE_BDL, VOICE_TYPE_FE_SLT, VOICE_TYPE_XIAOQIAN, VOICE_TYPE_XIAOMEI,\
//     VOICE_TYPE_XIAOQIANG, VOICE_TYPE_XIAOYAN, VOICE_TYPE_XIAOKUN, VOICE_TYPE_XIAORONG, VOICE_TYPE_XIAOYING, VOICE_TYPE_DE, VOICE_TYPE_EN_029, VOICE_TYPE_EN_GB, \
//     VOICE_TYPE_EN_GB_SCOTLAND, VOICE_TYPE_EN_GB_X_GBCLAN, VOICE_TYPE_EN_GB_X_GBCWMD, VOICE_TYPE_EN_GB_X_RP, VOICE_TYPE_EN_US, VOICE_TYPE_FR_BE, \
//     VOICE_TYPE_FR_CH, VOICE_TYPE_FR_FR, VOICE_TYPE_IT, VOICE_TYPE_JA, VOICE_TYPE_RU, VOICE_TYPE_RU_LV, VOICE_TYPE_ES, VOICE_TYPE_ES_419, \
//     VOICE_TYPE_AR, VOICE_TYPE_PT, VOICE_TYPE_PT_BR, VOICE_TYPE_PL, VOICE_TYPE_KO, VOICE_TYPE_MY, VOICE_TYPE_NE, VOICE_TYPE_VI, VOICE_TYPE_JAPANESE, \
//     VOICE_TYPE_AHW, VOICE_TYPE_SLT};

#define MAXLEN_TTS_TEXT (1000)




T_DjiReturnCode CziMegaphone_Init(void)
{
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

E_DjiWidgetSpeakerPlayMode CziMegaphone_GetPlayMode(void)
{
    return WidgetCziSpeaker_GetPlayMode();
}


T_DjiReturnCode CziMegaphone_AddMusic(const char *filename)
{
    T_ProtRspData rsp = {0x00};
    unsigned char input_data[MAXLEN_FILE_NAME + 3];
    input_data[0] = 0x00;
    input_data[1] = DIR_TYPE_MUSIC;     
    snprintf((char *)&input_data[2], MAXLEN_FILE_NAME, "%s", filename);
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_FILE_ADD, input_data, sizeof(input_data), &rsp);
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

int CziMegaphone_GetBasicInfo()
{
    T_ProtRspData rsp = {0x00};

    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_ASK_INFO, 0, 0, &rsp);
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

/*control music*/
T_DjiReturnCode CziMegaphone_CtrlMusicPlay(T_MusicListInfo MusicListInfo)
{
    printf("index: %d\n", MusicListInfo.musicListIndex);
    T_ProtRspData rsp = {0x00};
    unsigned char input_data[MAXLEN_FILE_NAME + 2];
    input_data[0] = MEDIA_CHANNEL_TYPE_MASTER;
    input_data[1] = MEDIA_FILE_TYPE_MUSIC;

    snprintf(&input_data[2], MAXLEN_FILE_NAME, "%s", MusicListInfo.musicName);
    // input_data[2] = index;
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_PLAY, (unsigned char *)input_data, sizeof(input_data), &rsp);
    printf("pcmd is %d, len is %d\n", CZI_PROTOCOL_COMMON_CMD_MUSIC_PLAY, rsp.rspLen);
    /* set queue (psdk-->prot) */
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
}

T_DjiReturnCode CziMegaphone_CtrlMusicStop()
{
    T_ProtRspData rsp = {0x00};
    unsigned char input_data[2] = {0x00};
    input_data[0] = MEDIA_CHANNEL_TYPE_MASTER;
    input_data[1] = 0xF3;
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_STATE, (unsigned char *)input_data, sizeof(input_data), &rsp);
    s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_IDEL;

    /* set queue (psdk-->prot) */
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
}

T_DjiReturnCode CziMegaphone_DeleteMusic(void *arg, int index)
{
    T_ProtRspData rsp = {0x00};
    unsigned char input_data[MAXLEN_FILE_NAME + 1];
    input_data[0] = DIR_TYPE_MUSIC;
    // snprintf(&input_data[1], MAXLEN_FILE_NAME, "%s", filename);
    input_data[1] = index - 1;
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_FILE_DELETE, (unsigned char *)input_data, sizeof(input_data), &rsp);

    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

int CziMegaphone_GetMusicSum(T_MusicListInfo *MusicListInfo)
{
    MusicListInfo->sum = 0;
    
    char pathList[MAXLEN_FILEPATH];
    memset(pathList, 0x00, MAXLEN_FILEPATH);
    memcpy(pathList, FILEPATH_MUSIC_LIST, MAXLEN_FILEPATH);
   
    FILE *fp = fopen(pathList, "r");
    if(fp == NULL) {
        printf("open %s failed!\n", pathList);
        return -1;
    }  
    char musicname[MAXLEN_FILENAME_MUSIC];

    while(!feof(fp)) {
        memset(musicname, 0x00, MAXLEN_FILENAME_MUSIC);
        fgets(musicname, MAXLEN_FILENAME_MUSIC, fp);
        if(strlen(musicname)) {
            MusicListInfo->sum++;
        }
    }
    fclose(fp);
    printf("get music sum is %d \n", MusicListInfo->sum);
    return 0;
}

int CziMegaphone_GetMusicName(T_MusicListInfo *MusicListInfo, int index)
{

    if(index > MusicListInfo->sum || index <= 0 ) {
        printf("music index(index : %d) is error! sum is %d\n", index, MusicListInfo->sum);
        return -1;
    }
    MusicListInfo->musicListIndex = index;

    char pathList[MAXLEN_FILEPATH];
    memset(pathList, 0x00, MAXLEN_FILEPATH);
    memcpy(pathList, FILEPATH_MUSIC_LIST, MAXLEN_FILEPATH);
  
    FILE *fp = fopen(pathList, "r");
    if(fp == NULL) {
        printf("open %s failed!\n", pathList);
        return -1;
    }
    int sum = 1;
    char musicname[MAXLEN_FILENAME_MUSIC];

    while(!feof(fp)) {
        memset(musicname, 0x00, MAXLEN_FILENAME_MUSIC);
        fgets(musicname, MAXLEN_FILENAME_MUSIC, fp);
        int len = strlen(musicname); /*unnecessary '\r' '\n'*/
        if(len) {
            if(sum == index) {
                memset(MusicListInfo->musicName, 0x00, MAXLEN_FILENAME_MUSIC);
                if (musicname[len-1] == '\r' || musicname[len-1] == '\n') {
                    musicname[len-1] = '\0';
                    len -= 1;
                    printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
                }            
            musicname[len] = '\0';
                memcpy(MusicListInfo->musicName, musicname, len);
                break;
            }
            sum++;
        }
    }
    fclose(fp);
    printf("music name is %s\n", MusicListInfo->musicName);
    return 0;
}
/*music  ctorl stop*/

T_DjiReturnCode CziMegaphone_SetVolume(uint8_t volume)
{
    // if(volume) {
    //     volume = volume/10 + 1;
    //     volume = volume * 10;
    //     if(volume > 100)
    //         volume = 100;        
    // }
    if(volume == 0) 
    {
        unsigned char Amp = 0x41;
        CziWidget_GetRecordAmp(Amp);
    }
    printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
    // CziWidget_SetIndexValue(WIDGET_INDEX_VOLUME, volume);
    T_ProtRspData rsp;
    int realVolume = 0;
    MediaHandler_SetVolume(MEDIA_CHANNEL_TYPE_MASTER, volume, &realVolume);
    
    /* TODO packet protocol */
    unsigned char packet[2];
    packet[0] = MEDIA_CHANNEL_TYPE_MASTER;
    packet[1] = (unsigned char)realVolume;
    int len = sizeof(packet);

    printf("realvolume is %d\n", packet[1]);

    printf("!!!!!PSDK SEND  PROT  channel:%x    VOLUME:%d", packet[0], packet[1]);
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_SYS_VOLUME, (unsigned char*)packet, len, &rsp);
    
    /* set queue (psdk-->prot) */
    
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    // CziMegaphone_UpdateVolume(volume);
    CziWidget_SetIndexValue(WIDGET_INDEX_VOLUME, realVolume);
    s_speakerState.volume = realVolume;

    // s_speakerState.volume = volume;
    CziPayload_UpdateMediaWidget();

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

// only  CziMsdkHandler_TtsPlay  调用
void CziMegaphone_SetTtsVoiceType(const char voiceType, char *ttsType, char *newType)
{
    CziMegaphone_SetTtsVoice(voiceType);
    MediaHandler_GetTtsType(ttsVoice, ttsType);
    

    memset(newType, 0x00, sizeof(char));
    memcpy(newType, &ttsVoice, sizeof(char));
    printf("MediaHandler_GetTtsType ttsType:%x   newType:%x\n", ttsVoice, newType);
    // newType = ttsVoice;
    
}


void CziMegaphone_SetTtsVoice(char ttsVoic)
{   
    CziJsonHandler_Open(FILEPATH_JSON_STATE_TTS, 0);
    CziJsonHandler_WriteInt(JSON_KEY_TTS_VOICE, ttsVoic);
    // CziJsonHandler_WriteString(JSON_KEY_TTS_VOICE, ttsVoice);
    CziJsonHandler_Close(1);
    printf("!!!!!!!!!!!!!tts  voice adjust front ttsvoice:%x\n", ttsVoic);
    ttsVoice = CziMegaphone_TtsTypeAdjust(ttsVoic);
    printf("!!!!!!!!!!!!!tts  voice adjust back ttsvoice:%x\n", ttsVoice);

    

    if ( ttsVoice == VOICE_TYPE_FE_BDL) { // 映射到正常可用的语种
        ttsVoice = VOICE_TYPE_AHW;
    }
    else if ( ttsVoice == VOICE_TYPE_FE_SLT ) {
        ttsVoice = VOICE_TYPE_SLT;
    }

    char ttsType = MEDIA_TTS_IDLE;
    MediaHandler_GetTtsType(ttsVoice, &ttsType);

    T_TtsInfo tTtsInfo = {
        .ttsType   = ttsType,
        .voiceType = ttsVoice,
        .speedType = MEDIA_TTS_IDLE,
    };

    MediaHandler_HandleTtsInfo(MEDIA_CMD_WRITE, &tTtsInfo);
}

static int CziMegaphone_TtsTypeAdjust(char ttsVoice)
{
    int type = ttsVoice;

    printf("type is %d, ttsType[type] is %x\n",  type, ttstype[type]);

    return ttstype[type];
}

void CziMegaphone_SetTtsSpeed(char ttsSpeed)
{
    USER_LOG_INFO("tts speed: 0x%x\n", ttsSpeed);
    
    CziJsonHandler_Open(FILEPATH_JSON_STATE_TTS, 0);
    CziJsonHandler_WriteInt(JSON_KEY_TTS_SPEED, ttsSpeed);
    // CziJsonHandler_WriteString(JSON_KEY_TTS_SPEED, ttsSpeed);
    CziJsonHandler_Close(1);
    

    T_TtsInfo tTtsInfo = {
        .ttsType   = MEDIA_TTS_IDLE,
        .voiceType = MEDIA_TTS_IDLE,
        .speedType = ttsSpeed
    };

    MediaHandler_HandleTtsInfo(MEDIA_CMD_WRITE, &tTtsInfo);
}
T_DjiReturnCode CziMegaphone_SetMediaMode(E_MediaMode MediaMode)
{
    printf("mode is %d\n", MediaMode);
    gMediaMode = MediaMode;
    WidgetCziSpeaker_GetMediaMode(gMediaMode);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

// void CziMegaphone_GetMediaMode(mode)
// {
//     int mode =gMediaMode;
// }

T_DjiReturnCode CziMegaphone_SetPlayMode(E_DjiWidgetSpeakerPlayMode playMode)
{
    printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
    CziWidget_SetIndexValue(WIDGET_INDEX_MUSIC_SINGLE_CYCLE, playMode);
    unsigned char cycleMode = 0;
    if(playMode)
        cycleMode = 0xC1;
    else 
        cycleMode = 0xC0;

    /* TODO packet protocol */
    unsigned char input_data[2] = {0x00};
    input_data[0] = MEDIA_CHANNEL_TYPE_MASTER;
    input_data[1] = cycleMode;
    T_ProtRspData rsp;
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_MODE, (unsigned char *)input_data, sizeof(input_data), &rsp);

    /* set queue (psdk-->prot) */
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    USER_LOG_INFO("SetPlayMode ***************");
    s_speakerState.playMode = playMode;
    // WidgetCziSpeaker_UpdatePlayMode(playMode);
    CziPayload_UpdateMediaWidget();
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

// 开机播报
T_DjiReturnCode CziMegaphone_StartBroadcast(char value)
{
    printf("  GETGET CziMegaphone_StartBroadcast value is %x\r\n",value);
    int startMute = 240;
    if (value) {
        startMute = 241;
    }
    CziJsonHandler_Open(FILEPATH_JSON_CONFIG_USR, 0);
    CziJsonHandler_WriteInt(JSON_KEY_STARTUP_MUTE, startMute);
    CziJsonHandler_Close(1);

}


//state
void CziMegaphone_SetSpeakerState(E_DjiWidgetSpeakerState speakerState)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    s_speakerState.state = speakerState;
}

T_DjiReturnCode CziMegaphone_GetSpeakerState(T_DjiWidgetSpeakerState *speakerState)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    *speakerState = s_speakerState;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode CziMegaphone_StartPlay(void)
{
    printf("s_speakerState.workMode is %d, s_speakerState.playMode is %d, s_speakerState.state is %d\n", s_speakerState.workMode, s_speakerState.playMode, s_speakerState.state);
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    if(s_speakerState.state == DJI_WIDGET_SPEAKER_STATE_PLAYING)
    {
        CziMegaphone_StopPlay();
        osalHandler->TaskSleepMs(200);
    }
    
        // return 0;
    s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_PLAYING;
    if (g_TtsMode == TTS_TEXT_MODE && s_speakerState.workMode == DJI_WIDGET_SPEAKER_WORK_MODE_TTS) {
        g_TtsMode = TTS_STREAM_MODE;
        if (WidgetCziSpeaker_GetTtsRunningState() == TTS_RUNNING) {
            printf("***************************** tts is running *************************\n");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
        } else {
            uint8_t cmd = 0xf3;
            CziMegaphone_SetTtsInfo(DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH, &cmd, 1);
            WidgetCziSpeaker_UpdateTtsRunningState(TTS_RUNNING);

        }
    } else {




    T_ProtRspData rsp = {0x00};
    unsigned char input_data[MAXLEN_FILE_NAME];
    input_data[0] = MEDIA_CHANNEL_TYPE_MASTER; 
    input_data[1] = MEDIA_FILE_TYPE_MUSIC;
    CziMegaphone_AddMusic(gs_RecordFilename);    
    CziPayload_UpdateMediaWidget();
    CziMegaphone_SetPlayMode(s_speakerState.playMode);
    osalHandler->TaskSleepMs(200);
    snprintf(&input_data[2], MAXLEN_FILE_NAME, "%s", gs_RecordFilename);
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_PLAY, (unsigned char *)input_data, sizeof(input_data), &rsp);
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    }
    CziMegaphone_SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_PLAYING);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}



T_DjiReturnCode CziMegaphone_StopPlay(void)
{
    CziMegaphone_CtrlMusicStop();
    CziMegaphone_SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_IDEL); /////////////////
    s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_IDEL;
    CziPayload_UpdateMediaWidget();
    WidgetCziSpeaker_UpdateTtsRunningState(TTS_IDLE);


    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
//窗口  TTS  和  VOICE 选择
T_DjiReturnCode CziMegaphone_SetWorkMode(E_DjiWidgetSpeakerWorkMode workMode)
{

    USER_LOG_INFO("Set widget speaker work mode: %d", workMode);
    if (!workMode) {
        s_speakerState.workMode = DJI_WIDGET_SPEAKER_WORK_MODE_TTS;
    }else {
        s_speakerState.workMode = DJI_WIDGET_SPEAKER_WORK_MODE_VOICE;
    }
        
        // CziMegaphone_SetMediaMode(REALTIME_MODE);

    WidgetCziSpeaker_UpdateWorkMode(workMode);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


// void CziMegaphone_SetTtsInfo(E_DjiWidgetTransmitDataEvent event, const char *input_data, uint16_t size)
// {
//     s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_PLAYING;
//     T_ProtRspData rsp = {0x00};
//     T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
//     E_ProtocolCommandAll pcmd = CZI_PROTOCOL_COMMON_CMD_TTS_PLAY;
//     int totalen;
//     // s_speakerState.workMode = DJI_WIDGET_SPEAKER_WORK_MODE_TTS;
//     printf("event is %d, size is %d\n", event, size);
//     if(event == DJI_WIDGET_TRANSMIT_DATA_EVENT_START ) {
//         if(s_speakerState.playMode == DJI_WIDGET_SPEAKER_PLAY_MODE_LOOP_PLAYBACK){
//             CziMegaphone_SetPlayTTSMode(DJI_WIDGET_SPEAKER_PLAY_MODE_SINGLE_PLAY);
//             osalHandler->TaskSleepMs(200);
//         }
//         // }else    
//         //     CziMegaphone_SetPlayMode(DJI_WIDGET_SPEAKER_PLAY_MODE_SINGLE_PLAY);
            
//         osalHandler->TaskSleepMs(200);

//         char handle_data[4] = {0x00};
//         PT_TtsInfo ptTtsInfo;
//         MediaHandler_HandleTtsInfo(MEDIA_CMD_READ, ptTtsInfo);
//         // memcpy(handle_data, input_data, 1);
//         handle_data[0] = input_data[0];
//         handle_data[1] = ptTtsInfo->ttsType;
//         handle_data[2] = ptTtsInfo->voiceType;
//         handle_data[3] = ptTtsInfo->speedType;
//         // size = 4;
//         totalen = Prot_LongFormatPack(pcmd, (unsigned char *)handle_data, sizeof(handle_data), &rsp);
//     } else if(event == DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT ) {
//         if(s_speakerState.playMode == DJI_WIDGET_SPEAKER_PLAY_MODE_LOOP_PLAYBACK){
//             CziMegaphone_SetPlayTTSMode(DJI_WIDGET_SPEAKER_PLAY_MODE_LOOP_PLAYBACK);
//             osalHandler->TaskSleepMs(200);
//         }


//         static char data[MAXLEN_TTS_TEXT] = {0x00};
//         data[0] = 0xF2;
//         memcpy(&data[1], input_data, size);
//         totalen = Prot_LongFormatPack(pcmd, (unsigned char *)data, size+1, &rsp);
//         // CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, (char *)rsp.rspData, rsp.rspLen);
//         printf("size is %d\n", size);
//     }
//     else {
//         // memcpy(&data[1], input_data, size);
//         totalen = Prot_LongFormatPack(pcmd, (unsigned char *)input_data, size, &rsp);
//         CziPayload_UpdateMediaWidget();
//     }    

//     CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, (char *)rsp.rspData, rsp.rspLen);

// }


#define MAXLEN_TTS_TEXT (1000)

void CziMegaphone_SetTtsInfo(E_DjiWidgetTransmitDataEvent event, const char *input_data, uint16_t size)
{
    g_TtsMode = TTS_TEXT_MODE;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_ProtRspData rsp = {0x00};
    E_ProtocolCommandAll pcmd = CZI_PROTOCOL_COMMON_CMD_TTS_PLAY;
    int totalen;
    // s_speakerState.workMode = DJI_WIDGET_SPEAKER_WORK_MODE_TTS;
    printf("event is %d, size is %d\n", event, size);
    if(event == DJI_WIDGET_TRANSMIT_DATA_EVENT_START ) {
        char handle_data[4] = {0x00};
        T_TtsInfo ptTtsInfo;
        MediaHandler_HandleTtsInfo(MEDIA_CMD_READ, &ptTtsInfo);
        memcpy(handle_data, input_data, 1);
        handle_data[0] = input_data[0];
        handle_data[1] = ptTtsInfo.ttsType;
        handle_data[2] = ptTtsInfo.voiceType;
        handle_data[3] = ptTtsInfo.speedType;
        // size = 4;
        totalen = Prot_LongFormatPack(pcmd, (unsigned char *)handle_data, sizeof(handle_data), &rsp);
    
        if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
            CziMegaphone_SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_TRANSMITTING);
        }
    } else if(event == DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT ) {
        // if(s_speakerState.playMode == DJI_WIDGET_SPEAKER_PLAY_MODE_LOOP_PLAYBACK){
        //     CziMegaphone_SetPlayTTSMode(DJI_WIDGET_SPEAKER_PLAY_MODE_LOOP_PLAYBACK);
        //     osalHandler->TaskSleepMs(200);
        // }
        static char data[MAXLEN_TTS_TEXT] = {0x00};
        data[0] = 0xF2;
        
        memcpy(&data[1], input_data, size);
        printf("@@@@@@@@@@@@@@@@@%s\n", &data[1]);
        totalen = Prot_LongFormatPack(pcmd, (unsigned char *)data, size+1, &rsp);
        // CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, (char *)rsp.rspData, rsp.rspLen);
        printf("size is %d\n", size);

        if (s_speakerState.state != DJI_WIDGET_SPEAKER_STATE_PLAYING) {
            CziMegaphone_SetSpeakerState(DJI_WIDGET_SPEAKER_STATE_TRANSMITTING);
        }
    }
    else {
        totalen = Prot_LongFormatPack(pcmd, (unsigned char *)input_data, size, &rsp);
        CziPayload_UpdateMediaWidget();
    }    

    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, (char *)rsp.rspData, rsp.rspLen);

}

void CziMegaphone_SetPlayInfo(E_SpeakerMode speakerMode, const char *musicName)
{
    g_speakerMode = speakerMode;
    memset(gs_RecordFilename, 0x00, MAXLEN_FILE_NAME);
    memcpy(gs_RecordFilename, musicName, MAXLEN_FILE_NAME);
    printf("gs_RecordFilename is %s\n", gs_RecordFilename);
    
}

void CziMegaphone_SendAudioInfo(E_DjiWidgetTransmitDataEvent event, const char *input_data, uint16_t size)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    E_ProtocolCommandAll pcmd;
    switch (event)
    {
        case DJI_WIDGET_TRANSMIT_DATA_EVENT_TRANSMIT:
        {
            pcmd = CZI_PROTOCOL_COMMON_DATA_STREAM_UP;
            break;
        }
        case DJI_WIDGET_TRANSMIT_DATA_EVENT_START:
        {
            // s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_PLAYING;
            pcmd = CZI_PROTOCOL_COMMON_CMD_STREAM_UP;
            break;
        }
        case DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH:
        {
            // s_speakerState.state = DJI_WIDGET_SPEAKER_STATE_IDEL;
            pcmd = CZI_PROTOCOL_COMMON_CMD_STREAM_UP;
            break;
        }
        default:
            break;
    }
    
    // printf("gMediaMode is %d\n", gMediaMode);
    if(gMediaMode == REALTIME_MODE && g_speakerMode == IS_RECORD ) {
        T_ProtRspData rsp = {0x00};
        
        printf("!!!!!!!!!!!!!psdk send media audio data %d\n", size);
        int totalen = Prot_LongFormatPack(pcmd, (unsigned char *)input_data, size, &rsp);
        // for(int i=0; i<size;i++) {
        //      printf("!!!!!!!!!!!!!psdk send media audio data %d:%x \n", i, input_data[i]);
        // }
        /* set queue (psdk-->prot) */
        // CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
        CziTransmission_SendDataByMailbox(rsp.rspData, rsp.rspLen);
    }
    // }else if(mode == DJI_WIDGET_SPEAKER_WORK_MODE_TTS && event == DJI_WIDGET_TRANSMIT_DATA_EVENT_FINISH) {
    //     printf("!!!!!!!!!!!\n");
    //     osalHandler->TaskSleepMs(200);
    //     // CziMegaphone_StartPlay();
    // }
    
}

//psdk handler  2 diaoy
int CziMegaphone_UpdateVolume(int volume)
{
    s_speakerState.volume = volume;
}

//psdk handler  2 diaoy
int CziMegaphone_UpdateLoopModel(int loop_model)
{
    printf("!!!!!!!!!!loop_model : %d", loop_model);  
    int loop_control = loop_model - 0xc0;
    if(loop_control == 0x00)
    {
        s_speakerState.playMode = DJI_WIDGET_SPEAKER_PLAY_MODE_SINGLE_PLAY;
    }
    else if(loop_control == 0x01)
    {
        s_speakerState.playMode = DJI_WIDGET_SPEAKER_PLAY_MODE_LOOP_PLAYBACK;
    }
    else
    {
        printf("loop_control is lost,loop_control is %d",loop_control);
    }

    unsigned char input_data[2] = {0x00};
    input_data[0] = MEDIA_CHANNEL_TYPE_MASTER;
    input_data[1] = loop_model;
    printf("loop model is %d\n", loop_model);
    T_ProtRspData rsp;
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_MODE, (unsigned char *)input_data, sizeof(input_data), &rsp);

    /* set queue (psdk-->prot) */
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
}

T_DjiReturnCode CziMegaphone_SetAiVideoState(int AiVideostreamState)
{
    T_ProtRspData rsp;
    unsigned char packet[3];
    int streamstate;
    if (AiVideostreamState == 1){
        streamstate = 0xF1;
    }
    else if (AiVideostreamState == 0){
        CziMegaphone_CtrlMusicStop();
        streamstate = 0xF2;
    }
    else{
        printf("Error AiVideostreamState is %d\n",AiVideostreamState);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
    packet[0] = 0x00;
    packet[1] = streamstate;
    packet[2] = 0x00;
    int len = sizeof(packet);
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_AI_VIDEO, (unsigned char*)packet, len, &rsp);
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    USER_LOG_INFO("set volume callback ***************");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

//add

T_DjiReturnCode CziMegaphone_SetPlayTTSMode(E_DjiWidgetSpeakerPlayMode playMode)
{
    printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
    CziWidget_SetIndexValue(WIDGET_INDEX_MUSIC_SINGLE_CYCLE, playMode);
    unsigned char cycleMode = 0;
    if(playMode)
        cycleMode = 0xc1;
    else 
        cycleMode = 0xc0;

    /* TODO packet protocol */
    unsigned char input_data[2] = {0x00};
    input_data[0] = MEDIA_CHANNEL_TYPE_MASTER;
    input_data[1] = cycleMode;
    T_ProtRspData rsp;
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_MODE, (unsigned char *)input_data, sizeof(input_data), &rsp);

    /* set queue (psdk-->prot) */
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    // s_speakerState.playMode = playMode;
    // WidgetCziSpeaker_UpdatePlayMode(playMode);

    // Payload_UpdateMediaWidget();
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


int CziMegaphone_SetBreathLight(char state)
{
    CziJsonHandler_Open(FILEPATH_JSON_CONFIG_USR, 0);
    CziJsonHandler_WriteInt(JSON_KEY_BREATH_LIGHT, state);
    CziJsonHandler_Close(1);
    return 0;
}