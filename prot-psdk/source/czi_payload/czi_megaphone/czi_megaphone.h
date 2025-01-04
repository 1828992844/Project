/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CZI_MEGAHPONE_H
#define CZI_MEGAHPONE_H

/* Includes ------------------------------------------------------------------*/
#include "dji_typedef.h"
#include "dji_widget.h"
#include "widget_czi_speaker/widget_czi_speaker.h"
#include "../../psdk_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/


/* Exported types ------------------------------------------------------------*/
// typedef enum _DirType {
//     DIR_TYPE_MUSIC      = 0x00
// } E_DirType;

typedef struct _MediaListInfo
{
    int sum;
    int musicListIndex;
    char musicName[MAXLEN_FILE_NAME];
    int mediastats;
    // E_DirType eDirType;
} T_MusicListInfo, *PT_MusicListInfo;

typedef struct _MegaphoneInfo
{
    int mediaStats; /*open or close*/
    int mediaMode; /*cycle*/
    int volume;
} T_MegaphoneInfo, *PT_MegaphoneInfo;

typedef enum _MediaMode{
    RECORD_MODE   = 0x00,
    REALTIME_MODE = 0x01,
    TTS_MODE      = 0x02
}E_MediaMode;
typedef enum _TtsMode{
    TTS_STREAM_MODE    = 0x00,
    TTS_TEXT_MODE,
}E_TtsMode;

/* Exported functions --------------------------------------------------------*/
T_DjiReturnCode CziMegaphone_Init(void);
T_DjiReturnCode CziMegaphone_StopPlay(void);
T_DjiReturnCode CziMegaphone_SetPlayMode(E_DjiWidgetSpeakerPlayMode playMode);
T_DjiReturnCode CziMegaphone_SetPlayTTSMode(E_DjiWidgetSpeakerPlayMode playMode);
E_DjiWidgetSpeakerPlayMode CziMegaphone_GetPlayMode(void);

T_DjiReturnCode CziMegaphone_SetVolume(uint8_t volume);
T_DjiReturnCode CziMegaphone_SetMediaMode(E_MediaMode MediaMode);
int CziMegaphone_SetBreathLight(char state);


T_DjiReturnCode CziMegaphone_StartPlay(void);
T_DjiReturnCode CziMegaphone_AddMusic(const char *filename);
void CziMegaphone_SetTtsInfo(E_DjiWidgetTransmitDataEvent event, const char *input_data, uint16_t size);
void CziMegaphone_SetTtsSpeed(char ttsSpeed);
void CziMegaphone_SetTtsVoice(char ttsVoice);

void CziMegaphone_SetPlayInfo(E_SpeakerMode speakerMode, const char *musicName);
void CziMegaphone_SendAudioInfo(E_DjiWidgetTransmitDataEvent event, const char *input_data, uint16_t size);
int CziMegaphone_GetMusicSum(T_MusicListInfo *MusicListInfo);
int CziMegaphone_GetMusicName(T_MusicListInfo *MusicListInfo, int index);

/*control music*/
T_DjiReturnCode CziMegaphone_CtrlMusicPlay(T_MusicListInfo MusicListInfo);
T_DjiReturnCode CziMegaphone_CtrlMusicStop();

// void CziMegaphone_SetTtsVoiceType(const char voiceType, char ttsType);
void CziMegaphone_SetTtsVoiceType(const char voiceType, char *ttsType, char *newType);

T_DjiReturnCode CziMegaphone_SetWorkMode(E_DjiWidgetSpeakerWorkMode workMode);
T_DjiReturnCode CziMegaphone_DeleteMusic(void *arg, int index);
T_DjiReturnCode CziMegaphone_StartBroadcast(char value);
T_DjiReturnCode CziMegaphone_GetSpeakerState(T_DjiWidgetSpeakerState *speakerState);
T_DjiReturnCode CziMegaphone_SetAiVideoState(int AiVideostreamState);

int CziMegaphone_GetDjiRecordName(const char *folderPath, T_MusicListInfo *MusicListInfo, int index);
int CziMegaphone_GetDjiRecordSum(const char *folderPath, T_MusicListInfo *MusicListInfo);
void CziMegaphone_SetSpeakerState(E_DjiWidgetSpeakerState speakerState);
int CziMegaphone_GetBasicInfo();
int CziMegaphone_UpdateVolume(int volume);
int CziMegaphone_UpdateLoopModel(int loop_model);
#ifdef __cplusplus
}
#endif

#endif // CZI_MEGAHPONE_H