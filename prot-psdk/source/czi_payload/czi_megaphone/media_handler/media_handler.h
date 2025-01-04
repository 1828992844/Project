/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MEDIA_HANDLER_H
#define MEDIA_HANDLER_H

/* Includes ------------------------------------------------------------------*/


#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/

#define CMD_MEDIA_DATA_READ    (0x00)
#define CMD_MEDIA_DATA_WRITE   (0x01)
#define MAXLEN_FILENAME_MUSIC  (128)

#define MEDIA_TTS_IDLE         (-1)

/* Exported types ------------------------------------------------------------*/
typedef enum {
    TTS_TYPE_IFLYTEK = 0x00,
    TTS_TYPE_FESTIVAL,
    TTS_TYPE_ESPEAK,
    TTS_TYPE_OPEN_JTALK,
    TTS_TYPE_FLITE
} E_TtsType;

typedef enum {
    //IFLYTEK
    VOICE_TYPE_XIAOKUN = 0x00,
    VOICE_TYPE_XIAOQIAN,
    VOICE_TYPE_XIAORONG,
    VOICE_TYPE_XIAOYING,
    VOICE_TYPE_XIAOFENG,
    VOICE_TYPE_XIAOMEI,
    VOICE_TYPE_XIAOQIANG,
    VOICE_TYPE_XIAOYAN,

    // VOICE_TYPE_XIAOYAN=0x00,
    // VOICE_TYPE_XIAOFENG,
    // VOICE_TYPE_XIAOQIAN,
    // VOICE_TYPE_XIAOMEI,
    // VOICE_TYPE_XIAOQIANG,
    // VOICE_TYPE_XIAOKUN,
    // VOICE_TYPE_XIAORONG,
    // VOICE_TYPE_XIAOYING,




    
    //FESTIVAL
    VOICE_TYPE_FE_BDL,    //man
    VOICE_TYPE_FE_SLT,    //women
    
    //ESPEAK
    VOICE_TYPE_DE, //German
    VOICE_TYPE_EN_029, //English_(Caribbean)
    VOICE_TYPE_EN_GB, //English_(Great_Britain)
    VOICE_TYPE_EN_GB_SCOTLAND, //English_(Scotland)
    VOICE_TYPE_EN_GB_X_GBCLAN, //English_(Lancaster)
    VOICE_TYPE_EN_GB_X_GBCWMD, //English_(West_Midlands)
    VOICE_TYPE_EN_GB_X_RP, //English_(Received_Pronunciation)
    VOICE_TYPE_EN_US, //English_(America)
    VOICE_TYPE_FR_BE, //French_(Belgium)
    VOICE_TYPE_FR_CH, //French_(Switzerland)
    VOICE_TYPE_FR_FR, //French_(France)
    VOICE_TYPE_IT, //Italian
    VOICE_TYPE_JA, //Japanese
    VOICE_TYPE_RU, //Russian
    VOICE_TYPE_RU_LV, //Russian_(Latvia)
    VOICE_TYPE_ES, //Spanish_(Spain)
    VOICE_TYPE_ES_419, //Spanish_(Latin_America)
    VOICE_TYPE_AR, //Arabic
    VOICE_TYPE_PT, //Portuguese_(Portugal)
    VOICE_TYPE_PT_BR, //Portuguese_(Brazil)
    VOICE_TYPE_PL, //Polish
    VOICE_TYPE_KO, //Korean
    VOICE_TYPE_MY, //Myanmar_(Burmese)
    VOICE_TYPE_NE, //Nepali
    VOICE_TYPE_VI, //Vietnamese_(Northern)

    // OPEN_JTALK
    VOICE_TYPE_JAPANESE,

    // FLITE
    VOICE_TYPE_AHW,   //English_(America) man
    VOICE_TYPE_SLT    //English_(America) women

} E_VoiceType;



typedef enum {
    SPEED_TYPE_SLOWEST = 0x00,
    SPEED_TYPE_SLOWER,
    SPEED_TYPE_NORMAL,
    SPEED_TYPE_FASTER,
    SPEED_TYPE_FASTEST
} E_SpeedType;

typedef struct _TtsInfo {
    E_TtsType ttsType;
    E_VoiceType voiceType;
    E_SpeedType speedType;
} T_TtsInfo, *PT_TtsInfo;

typedef enum _MediaChannelType {
    MEDIA_CHANNEL_TYPE_MASTER     = 0x00,
    MEDIA_CHANNEL_TYPE_BACKGROUND = 0x01
} E_MediaChannelType;

typedef enum _MediaFileType {
    MEDIA_FILE_TYPE_MUSIC      = 0x00,
    MEDIA_FILE_TYPE_DJI_RECORD = 0x01
} E_MediaFileType;

typedef struct _MusicInfo {
    E_MediaFileType eMediaFileType;
    char filename[MAXLEN_FILENAME_MUSIC];
} T_MusicInfo, *PT_MusicInfo;

typedef enum _MediaCmd {
    MEDIA_CMD_READ  = 0x00,
    MEDIA_CMD_WRITE = 0x01
} E_MediaCmd;



/* Exported functions --------------------------------------------------------*/
void MediaHandler_GetTtsType(const char voiceType, char *ttsType);
void MediaHandler_SetVolume(E_MediaChannelType eMediaChannelType, int scaleVolume, int *realVolume);
void MediaHandler_HandleMusicInfo(E_MediaCmd eMediaCmd, PT_MusicInfo ptMusicInfo);
void MediaHandler_HandleTtsInfo(E_MediaCmd eMediaCmd, PT_TtsInfo ptTtsInfo);

#ifdef __cplusplus
}
#endif

#endif // MEDIA_HANDLER_H