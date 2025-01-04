/**
 ********************************************************************
 * @file    test_widget_speaker.h
 * @brief   This is the header file for "test_widget_speaker.c", defining the structure and
 * (exported) function prototypes.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef WIDGET_CZI_SPEAKER_H
#define WIDGET_CZI_SPEAKER_H

/* Includes ------------------------------------------------------------------*/
#include "dji_widget.h"
// #include "../usermodule/czi_psdk_handler.h"
#ifdef __cplusplus
extern "C" {
#endif


/* Exported types ------------------------------------------------------------*/
typedef enum{
    IS_RECORD = 0,
    IS_UPLOAD_RECORD = 1,
    IS_TTS_AUDIO = 2
} E_SpeakerMode;

typedef enum  {
    TTS_IDLE      = 0x00,
    TTS_RUNNING   = 0x01
}  E_TtsRunningState;

typedef struct {
    uint8_t *buffer;
    size_t head;
    size_t tail;
    size_t count;
    size_t capacity;
} CircularBuffer;

typedef struct
{
    void (*SpeakerSetTtsContent)(E_DjiWidgetTransmitDataEvent event, const char *input_data, uint16_t size);
    void (*SpeakerSetPlayInfo)(E_SpeakerMode speakerMode, const char *musicName);
    void (*SpeakerSendAudioInfo)(E_DjiWidgetTransmitDataEvent event, const char *input_data, uint16_t size);
}T_CziSpeakerPlay;

/* Exported constants --------------------------------------------------------*/
T_DjiReturnCode WidgetCziSpeaker_WidgetSpeakerStartService(T_DjiWidgetSpeakerHandler *s_speakerHandler);

/**
 * @brief 更新喊话器模块音量
 * @param {uint8_t} volume 音量值
 * @return {*}
 */
T_DjiReturnCode WidgetCziSpeaker_UpdateVolume(uint8_t volume);
/**
 * @brief 更新喊话器播放模式控件状态
 * @param {E_DjiWidgetSpeakerPlayMode} playMode 模式
 * @return {*}
 */
T_DjiReturnCode WidgetCziSpeaker_UpdatePlayMode(E_DjiWidgetSpeakerPlayMode playMode);
E_DjiWidgetSpeakerPlayMode WidgetCziSpeaker_GetPlayMode(void);
/**
 * @brief 更新喊话器模块播放状态
 * @param {E_DjiWidgetSpeakerState} state 状态
 * @return {*}
 */
T_DjiReturnCode WidgetCziSpeaker_UpdateState(E_DjiWidgetSpeakerState state);
/**
 * @brief 注册czi的播放控制模块
 * @param {T_CziSpeakerPlay} *cziSpeakerPlay 结构体，同时注册信息反馈回调函数
 * @return {*}
 */
T_DjiReturnCode WidgetCziSpeaker_RegistPlay(T_CziSpeakerPlay *cziSpeakerPlay);

T_DjiReturnCode WidgetCziSpeaker_UpdateWorkMode(E_DjiWidgetSpeakerWorkMode workMode);
E_TtsRunningState WidgetCziSpeaker_GetTtsRunningState(void);
T_DjiReturnCode WidgetCziSpeaker_UpdateTtsRunningState(E_TtsRunningState ttsRunningState);

void init_circular_buffer(CircularBuffer *cb, size_t capacity);
void free_circular_buffer(CircularBuffer *cb) ;
bool enqueue(CircularBuffer *cb, const uint8_t *data, size_t frame_count);
bool dequeue(CircularBuffer *cb, uint8_t *data, size_t frame_count);


int WidgetCziSpeaker_GetErro(uint32_t erro);
int WidgetCziSpeaker_GetMediaMode(int mode);

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif // TEST_WIDGET_SPEAKER_H
/************************ (C) COPYRIGHT DJI Innovations *******END OF FILE******/
