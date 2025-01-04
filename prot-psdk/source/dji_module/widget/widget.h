/**
 ********************************************************************
 * @file    test_widget.h
 * @brief   This is the header file for "test_widget.c", defining the structure and
 * (exported) function prototypes.
 *
 * @copyright (c) 2021 DJI. All rights reserved.
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
#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H

/* Includes ------------------------------------------------------------------*/
#include <dji_typedef.h>
#include <dji_widget.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/
#define MAXLEN_FILENAME_MUSIC   (128)
/* Exported types ------------------------------------------------------------*/
typedef T_DjiReturnCode (*cb_widgetCziServeice)(E_DjiWidgetType widgetType, uint32_t index, int32_t value);
typedef enum _WidgetIndex {
    WIDGET_INDEX_MUSIC_CONTROL = 0x00,
    WIDGET_INDEX_FLASH_SWITCH,
    // WIDGET_INDEX_ALARM_BLOW,

    WIDGET_INDEX_MODULE_SWITCH,
    
    // WIDGET_INDEX_GIMBAL_ANGLE,   
    // WIDGET_INDEX_LIGHT_SPOT_AI,


    WIDGET_INDEX_VOLUME,
    WIDGET_INDEX_MUSIC_SINGLE_CYCLE,
    // WIDGET_INDEX_MUSIC_DIR_SELECT,
    WIDGET_INDEX_DELETE_MUSIC,
    WIDGET_INDEX_TTS_VOICE,
    WIDGET_INDEX_TTS_SPEED,
    WIDGET_INDEX_BREATHING_LIGHT,
    WIDGET_INDEX_VERSION_BROADCAST,


    // WIDGET_INDEX_LOCKGIMBAL,
    // WIDGET_INDEX_LEFTPITCHADJ,
    // WIDGET_INDEX_RIGHTPITCHADJ,
    // WIDGET_INDEX_SAVEADJ,





} E_WidgetIndex;

typedef enum _MusicCtrlIndex {
	MUSIC_CTRL_INDEX_LAST_SONG = 0x00,
	MUSIC_CTRL_INDEX_NEXT_SONG,
	MUSIC_CTRL_INDEX_PLAY_SONG,
	MUSIC_CTRL_INDEX_STOP_SONG,
	MUSIC_CTRL_INDEX_CTRL_IDLE,
} E_MusicCtrlIndex;

typedef enum _WidgetCziServiceCmd {
    WIDGET_CZI_SERVICE_CMD_HANDLE = 0x00,
    WIDGET_CZI_SERVICE_CMD_IGNORE = 0x01
} E_WidgetCziServiceCmd;

typedef struct _WidgetCziMediaInfo
{
    int sum;
    int musicListIndex;
    char musicName[MAXLEN_FILENAME_MUSIC];
    int mediaStats;
    int mediaMode;
    int volume;
    int aLightSwitch;
    int breathLight;
    int aLarmSwitch; //alarm blow
    // int gLightBright;
    // int gLightSwitch;
}T_WidgetCziMediaInfo, *PT_WidgetCziMediaInfo;

typedef struct _WidgetTempValInfo{
    float TempVal1;
    float TempVal2;
    float TempVal3;
}T_WidgetTempValInfo, *PT_WidgetTempValInfo;

typedef struct _WidgetListInfo{
    char txtList[30];
}T_WidgetListInfo, *PT_WidgetListInfo;

static T_WidgetListInfo aLightListInfo_Cn[]= {
    {"关闭"},   
    {"红蓝爆闪"},
    {"红灯爆闪"},
    {"蓝灯爆闪"},
    {"黄灯爆闪"},
    {"绿灯爆闪"}
};

static T_WidgetListInfo aLightListInfo_En[]= {
    {"off"},   
    {"red and blue flash"},
    {"red flash"},
    {"blue flash"},
    {"yellow flash"},
    {"green flash"}
};

static T_WidgetListInfo aLarmListInfo_Cn[]= {
    {"关闭"},   
    {"一键警笛"}
};

static T_WidgetListInfo aLarmListInfo_En[]= {
    {"off"},   
    {"alarm blow"}
};

/* Exported functions --------------------------------------------------------*/
// void CziWidget_SetWidgetValue(uint32_t index, int32_t value);
// T_DjiReturnCode CziWidget_GetWidgetValue(uint32_t index, int32_t *value);
T_DjiReturnCode DjiTest_WidgetStartService(cb_widgetCziServeice widgetCziService);
T_DjiReturnCode DjiTest_WidgetSetConfigFilePath(const char *path);

T_DjiReturnCode DjiTestWidget_SetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t value,
                                                    void *userData);
T_DjiReturnCode DjiTestWidget_GetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t *value,
                                                    void *userData);
T_DjiReturnCode Widget_MediaListInfo(T_WidgetCziMediaInfo *WidgetCziMediaInfo);

void CziWidget_SetIndexValue(uint32_t index, int32_t value);
// void CziWidget_SetLightSwitch(int light_switch, int light_code);
void CziWidget_SetLightSwitch(int light_code);
void CziWidget_SetLightCode(int light_code);
void CziWidget_SetLoopModel(int loop_model);
void CziWidget_SetStartupMute(int startup_mute);
void CziWidget_GetIndexValue(uint32_t index, int32_t *value);
void CziWidget_SendMessage(char* value, char* flag);
void CziWidget_GetRecordAmp(unsigned char recordvalue);
// void CziWidget_SetAiVideoStream(int AiVideoState);
void UpdateAiDetectResult();
void CziWidget_UpdateBugleTempVal(float Val3);
void CziWidget_UpdateMainTempVal(float Val1, float Val2);

// void CziSetWidgetValueAndState(int index, int value, int state);
__attribute__((weak)) void DjiTest_WidgetLogAppend(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // TEST_WIDGET_H
/************************ (C) COPYRIGHT DJI Innovations *******END OF FILE******/
