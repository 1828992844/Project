#include <stdio.h>
#include <string.h>

#include "dji_logger.h"
#include "media_handler.h"

#define VOLUME_RATIO_SCALE_AND_REAL    (1)


void MediaHandler_GetTtsType(const char voiceType, char *ttsType)
{
    USER_LOG_INFO("MediaHandler_GetTtsType voiceType : 0x%x\n", voiceType);

    *ttsType = MEDIA_TTS_IDLE;

    // if(voiceType <= VOICE_TYPE_XIAOYAN) {
    //     *ttsType = TTS_TYPE_IFLYTEK;
    //     USER_LOG_INFO("TTS_TYPE_IFLYTEK\n");
    // }
    if(voiceType <= VOICE_TYPE_XIAOYAN) {
        *ttsType = TTS_TYPE_IFLYTEK;
        USER_LOG_INFO("TTS_TYPE_IFLYTEK\n");
    }
    else if ( voiceType == VOICE_TYPE_FE_BDL || voiceType == VOICE_TYPE_FE_SLT) {
        *ttsType = TTS_TYPE_FESTIVAL;
        USER_LOG_INFO("TTS_TYPE_FESTIVAL\n");
    }
    else if ( voiceType == VOICE_TYPE_JAPANESE || voiceType == VOICE_TYPE_JA) {
        *ttsType = TTS_TYPE_OPEN_JTALK;
        USER_LOG_INFO("TTS_TYPE_OPEN_JTALK\n");
    }
    else if (voiceType >= VOICE_TYPE_DE && voiceType <= VOICE_TYPE_VI) {
        *ttsType = TTS_TYPE_ESPEAK;
        USER_LOG_INFO("TTS_TYPE_ESPEAK\n");
    
    } else if (voiceType == VOICE_TYPE_AHW || voiceType == VOICE_TYPE_SLT) {
        *ttsType = TTS_TYPE_FLITE;
        USER_LOG_INFO("TTS_TYPE_FLITE\n");
    }
}


int MediaHandler_PlayFile(void)
{

}

// 音量
static int Media_MapVolume(int scaleVolume)
{
    int realVolume = (int)((float)scaleVolume * VOLUME_RATIO_SCALE_AND_REAL);

    return realVolume;
}

void MediaHandler_SetVolume(E_MediaChannelType eMediaChannelType, int scaleVolume, int *realVolume)
{
    if (eMediaChannelType == MEDIA_CHANNEL_TYPE_MASTER) {
        *realVolume = Media_MapVolume(scaleVolume);
    }
    else if (eMediaChannelType == MEDIA_CHANNEL_TYPE_BACKGROUND) {
        *realVolume = scaleVolume;
    }
}


void MediaHandler_HandleMusicInfo(E_MediaCmd eMediaCmd, PT_MusicInfo ptMusicInfo)
{
    static T_MusicInfo s_tMusicInfo;

    if ( MEDIA_CMD_READ ==  eMediaCmd ) {
        memset(ptMusicInfo, 0x00, sizeof(T_MusicInfo));
        memcpy(ptMusicInfo, &s_tMusicInfo, sizeof(T_MusicInfo));
    }
    else if ( MEDIA_CMD_WRITE == eMediaCmd ) {
        memset(&s_tMusicInfo, 0x00, sizeof(T_MusicInfo));
        memcpy(&s_tMusicInfo, ptMusicInfo, sizeof(T_MusicInfo));
    }
}

void MediaHandler_HandleTtsInfo(E_MediaCmd eMediaCmd, PT_TtsInfo ptTtsInfo)
{
    static T_TtsInfo s_tTtsInfo;

    if ( MEDIA_CMD_READ ==  eMediaCmd ) {
        memset(ptTtsInfo, 0x00, sizeof(T_TtsInfo));
        memcpy(ptTtsInfo, &s_tTtsInfo, sizeof(T_TtsInfo));
    }
    else if ( MEDIA_CMD_WRITE == eMediaCmd ) {
        if (ptTtsInfo->ttsType != MEDIA_TTS_IDLE)
            s_tTtsInfo.ttsType = ptTtsInfo->ttsType;

        if (ptTtsInfo->voiceType != MEDIA_TTS_IDLE)
            s_tTtsInfo.voiceType = ptTtsInfo->voiceType;

        if (ptTtsInfo->speedType != MEDIA_TTS_IDLE)
            s_tTtsInfo.speedType = ptTtsInfo->speedType;
    }
}