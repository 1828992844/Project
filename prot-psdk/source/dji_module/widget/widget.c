/**
 ********************************************************************
 * @file    test_widget.c
 * @brief
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

/* Includes ------------------------------------------------------------------*/
#include "widget.h"
#include <dji_widget.h>
#include <dji_logger.h>
#include "../utils/util_misc.h"
#include <dji_platform.h>
#include <stdio.h>
#include "dji_sdk_config.h"
#include "../../czi_payload/czi_protocol_handler/protocol_commonHandler.h"
#include <string.h>
#include "../../czi_payload/czi_json_handler/czi_json_handler.h"
#include "dji_aircraft_info.h"
#include "../../czi_payload/czi_psdk_handler/czi_psdk_handler.h"
#include "../../psdk_config.h"
#include <sys/stat.h> 
#include <time.h>

// #include "file_binary_array_list_en.h"

// #include "usermodule/czi_psdk_handler.h"
// #include "usermodule/cmailbox/czi_cmailbox.h"

/* Private constants ---------------------------------------------------------*/
#define WIDGET_DIR_PATH_LEN_MAX         (256)
#define WIDGET_TASK_STACK_SIZE          (2048)
#define MAX_STR_LENGTH 64
#define MAX_STR_COUNT 5
/* Private types -------------------------------------------------------------*/
// char message[DJI_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN] = {"czi test"};
T_WidgetCziMediaInfo gWidgetCziMediaInfo;
T_DjiAircraftVersion aircraftInfoVersion;
/* Private functions declaration ---------------------------------------------*/
static void *DjiTest_WidgetTask(void *arg);
T_DjiReturnCode DjiTestWidget_SetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t value,
                                                    void *userData);
T_DjiReturnCode DjiTestWidget_GetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t *value,
                                                    void *userData);

/* Private values ------------------------------------------------------------*/
static T_DjiTaskHandle s_widgetTestThread;
static bool s_isWidgetFileDirPathConfigured = true;
static char s_widgetFileDirPath[DJI_FILE_PATH_SIZE_MAX] = {0};
static T_WidgetTempValInfo gs_WidgetTempValInfo;

static const T_DjiWidgetHandlerListItem s_widgetHandlerList[] = {
    {0, DJI_WIDGET_TYPE_LIST,   DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {1, DJI_WIDGET_TYPE_LIST,   DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {2, DJI_WIDGET_TYPE_LIST,   DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL}, 
    {3, DJI_WIDGET_TYPE_SCALE,   DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {4, DJI_WIDGET_TYPE_SWITCH,   DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {5, DJI_WIDGET_TYPE_BUTTON,    DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {6, DJI_WIDGET_TYPE_LIST,   DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {7, DJI_WIDGET_TYPE_LIST,    DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {8, DJI_WIDGET_TYPE_SWITCH,   DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL}, 
    {9, DJI_WIDGET_TYPE_SWITCH,    DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
};

static const T_DjiWidgetHandlerListItem s_widgetNotAvailable[] = {
    {0, DJI_WIDGET_TYPE_BUTTON,         DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
};

static const char *s_widgetTypeNameArray[] = {
    "Unknown",
    "Button",
    "Switch",
    "Scale",
    "List",
    "Int input box"
};

static const uint32_t s_widgetHandlerListCount = sizeof(s_widgetHandlerList) / sizeof(T_DjiWidgetHandlerListItem);
static int32_t s_widgetValueList[sizeof(s_widgetHandlerList) / sizeof(T_DjiWidgetHandlerListItem)] = {0};
static int32_t s_widgetStateList[sizeof(s_widgetHandlerList) / sizeof(T_DjiWidgetHandlerListItem)] = {0};
// static T_DjiReturnCode (*Czi_SetWidgetValue)(E_DjiWidgetType widgetType, uint32_t index, int32_t value);

static const uint32_t s_widgetNotAvailableCount = 1;
static int32_t s_widgetValueListNotAvailable[1] = {0};

unsigned int g_getActivateState = -1;

static cb_widgetCziServeice gs_widgetCziServiceHandler = NULL;

/* Exported functions definition ---------------------------------------------*/
void CziWidget_SetValueAndState(uint32_t index, uint32_t value, int state){
    s_widgetValueList[index] = value;
    s_widgetStateList[index] = state;
}

void CziWidget_SetWidgetValue(uint32_t index, int32_t value)
{
    s_widgetValueList[index] = value;
}

T_DjiReturnCode CziWidget_GetWidgetValue(uint32_t index, int32_t *value)
{
    *value = s_widgetValueList[index];
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static void CziWidget_InitWidgetValue(void)   
{
    s_widgetValueList[WIDGET_INDEX_MUSIC_CONTROL] = MUSIC_CTRL_INDEX_CTRL_IDLE;
}

T_DjiReturnCode DjiTest_WidgetStartService(cb_widgetCziServeice widgetCziService)
{
    T_DjiReturnCode djiStat;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    // if(arg != NULL ) Czi_SetWidgetValue = arg;
    if (widgetCziService == NULL) {
        USER_LOG_ERROR("czi callback widget service is NULL");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
    
    gs_widgetCziServiceHandler = widgetCziService;

    // g_getActivateState = CziActivate_GetState();
    g_getActivateState =  0;

    //Step 1 : Init DJI Widget
    djiStat = DjiWidget_Init();
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Dji test widget init error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    //Step 2 : Set UI Config (Linux environment)
    char curFileDirPath[WIDGET_DIR_PATH_LEN_MAX];
    char tempPath[WIDGET_DIR_PATH_LEN_MAX * 2];
    djiStat = DjiUserUtil_GetCurrentFileDirPath(__FILE__, WIDGET_DIR_PATH_LEN_MAX, curFileDirPath);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Get file current path error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    if(g_getActivateState == 0){
        if (s_isWidgetFileDirPathConfigured == true) {
            snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX * 2, "widget_file/en_big_screen");
        } else {
            snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX * 2, "%swidget_file/en_big_screen", curFileDirPath);
        }
    }else{
        if (s_isWidgetFileDirPathConfigured == true) {
            snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX * 2, "widget_file/en_big_screen_not_activate");
        } else {
            snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX * 2, "%swidget_file/en_big_screen_not_activate", curFileDirPath);
        }
    }

    USER_LOG_INFO("En widgetPath:%s", tempPath);

    //set default ui config path
    djiStat = DjiWidget_RegDefaultUiConfigByDirPath(tempPath);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Add default widget ui config error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    //set ui config for English language
    djiStat = DjiWidget_RegUiConfigByDirPath(DJI_MOBILE_APP_LANGUAGE_ENGLISH,
                                             DJI_MOBILE_APP_SCREEN_TYPE_BIG_SCREEN,
                                             tempPath);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Add widget ui config error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    //set ui config for Chinese language
    if(g_getActivateState == 0){
        if (s_isWidgetFileDirPathConfigured == true) {
            snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX * 2, "widget_file/cn_big_screen");
        } else {
            snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX * 2, "%swidget_file/cn_big_screen", curFileDirPath);
        }
    }else{
        if (s_isWidgetFileDirPathConfigured == true) {
            snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX * 2, "widget_file/cn_big_screen_not_activate");
        } else {
            snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX * 2, "%swidget_file/cn_big_screen_not_activate", curFileDirPath);
        }
    }
    USER_LOG_INFO("Cn widgetPath:%s", tempPath);

    djiStat = DjiWidget_RegUiConfigByDirPath(DJI_MOBILE_APP_LANGUAGE_CHINESE,
                                             DJI_MOBILE_APP_SCREEN_TYPE_BIG_SCREEN,
                                             tempPath);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Add widget ui config error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    //Step 3 : Set widget handler list
    if(g_getActivateState == 0){
        djiStat = DjiWidget_RegHandlerList(s_widgetHandlerList, s_widgetHandlerListCount);
        if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Set widget handler list error, stat = 0x%08llX", djiStat);
            return djiStat;
        }
    }else{
        djiStat = DjiWidget_RegHandlerList(s_widgetNotAvailable, s_widgetNotAvailableCount);
        if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Set widget handler list error, stat = 0x%08llX", djiStat);
            return djiStat;
        }
    }

    //Step 4 : Run widget api sample task
    // if(cziProductActivationStatus){
        if (osalHandler->TaskCreate("user_widget_task", DjiTest_WidgetTask, WIDGET_TASK_STACK_SIZE, NULL,
                                    &s_widgetTestThread) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Dji widget test task create error.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
        }
    // }

    CziWidget_InitWidgetValue();

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode DjiTest_WidgetSetConfigFilePath(const char *path)
{
    memset(s_widgetFileDirPath, 0, sizeof(s_widgetFileDirPath));
    memcpy(s_widgetFileDirPath, path, USER_UTIL_MIN(strlen(path), sizeof(s_widgetFileDirPath) - 1));
    s_isWidgetFileDirPathConfigured = true;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

__attribute__((weak)) void DjiTest_WidgetLogAppend(const char *fmt, ...)
{

}

#ifndef __CC_ARM
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wformat"
#endif

#define MSG_MAX_LEN DJI_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN

char cziMessage[MSG_MAX_LEN] = {"Czi floatingwindow info"};

/* Private functions definition-----------------------------------------------*/

/*Checks whether the byte is Chinese or English*/
int Widget_CheckDyte(unsigned char *info, int right)/*Checks whether the byte is Chinese or English*/
{
    unsigned char str0 = info[right-3];
    unsigned char str1 = info[right-2];
    unsigned char str2 = info[right-1];

    if ((str0 & 0xF0) == 0xE0) {
        return 0;
    }
    if ((str1 & 0xF0) == 0xE0) {
        return 2;
    }
    if ((str2 & 0xF0) == 0xE0) {
        return 1;
    }
    return 0;

}
#define PRE_FRAME_LEN 26

char sn[20];
char software_version[20];
char WidgetInfo[1024*4] = {0x00};
unsigned char gs_RecordValue = 65;
static void   Widget_GenerateString(char *str, int count) {
    memset(str, 0x00, MAX_STR_LENGTH + 1);
    if(count == 65)
    {
        for (int i = 0; i < 6; i++) {
        str[i] = '.';
        }
        str[count] = '\0'; 
    }else
    {
        for (int i = 0; i < count; i++) {
        str[i] = '|';
        }
        str[count] = '\0';
    }
}

static int compareVersions(T_DjiAircraftVersion* version) {
    uint8_t targetVersion[4] = {0x10, 0x01, 0x00, 0x14};
    return memcmp(version, targetVersion, sizeof(targetVersion)) == 0;
}

#define STR_FIXED_MESSAGE    "监听"
static void *DjiTest_WidgetTask(void *arg)
{
    T_DjiReturnCode djiStat, returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_DjiMobileAppInfo mobile_info;
    int32_t volume;
    int aiDetectResult;
    USER_UTIL_UNUSED(arg);
    char *info = "我和我的祖国一刻也不能分割 无论我走到哪里都流出一首赞歌 我歌唱每一座高山我歌唱每一条河 袅袅炊烟小小村落路上一道辙 你用你那母亲的脉搏和我诉说 我的祖国和我像海和浪花一朵 浪是海的赤子海是那浪的依托 每当大海在微笑我就是笑的旋涡 我分担着海的忧愁分享海的欢乐 永远给我碧浪清波心中的歌";
    int infolen = strlen(info);
    int offset = 0;
    int frameLen = PRE_FRAME_LEN;
    int bytesum = 0;
    int videoStreamFlag = 0;
    int detectFlag;
    // srand(time(NULL));
    // int versionFlag = 0;
    // DjiAircraftInfo_GetAircraftVersion(&aircraftInfoVersion);
    // printf("Aircraft version is V%d.%d.%d.%d\n", aircraftInfoVersion.majorVersion,
    //                       aircraftInfoVersion.minorVersion, aircraftInfoVersion.modifyVersion,
    //                       aircraftInfoVersion.debugVersion);
    // if(compareVersions(&aircraftInfoVersion)){
    //     versionFlag = 1;
    // }

    while (1) {
        // osalHandler->TaskSleepMs(500);
        // UpdateAiDetectResult(&aiDetectResult);
        // CziWidget_GetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, &videoStreamFlag);
        detectFlag = videoStreamFlag && aiDetectResult;
        returnCode = DjiAircraftInfo_GetMobileAppInfo(&mobile_info);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("get mobile info error");
            return NULL;
        }

        memset(cziMessage, 0, MSG_MAX_LEN);
        // memset(&cziMessage[fixedMessageLen], 0, MSG_MAX_LEN-fixedMessageLen);

        /*loop*/

        // memcpy(cziMessage + strlen(cziMessage), &info[offset], frameLen);
        // bytesum = Widget_CheckDyte(cziMessage, frameLen);
        // cziMessage[frameLen-bytesum] = '\0';
        // offset += frameLen-bytesum;
        // if (offset >= infolen) {
        //     offset = 0;
        // }

        //get volume
        // CziWidget_GetIndexValue(WIDGET_INDEX_VOLUME, &volume);
        // snprintf(cziMessage + strlen(cziMessage), 255, "音量：%d\n", volume);
        /*Chinese and English switching*/
        if(mobile_info.appLanguage == DJI_MOBILE_APP_LANGUAGE_CHINESE) {
            // monitor
            char str[MAX_STR_LENGTH];
            Widget_GenerateString(str, gs_RecordValue);
            snprintf(cziMessage, 255, "监听:%s\n", str);

            

            snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "音频(%d/%d):%s\n", gWidgetCziMediaInfo.musicListIndex, gWidgetCziMediaInfo.sum, gWidgetCziMediaInfo.musicName);

            
            if (detectFlag){
                snprintf(cziMessage + strlen(cziMessage), 255, "%s\n", "AI识别当前场景为“人员检测”");
            }
            snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "警灯:%s\n", aLightListInfo_Cn[gWidgetCziMediaInfo.aLightSwitch].txtList);   
            // snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "警笛:%s\n", aLarmListInfo_Cn[gWidgetCziMediaInfo.aLarmSwitch].txtList);   
            snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "音量:%d\n", gWidgetCziMediaInfo.volume); 

            // snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "喇叭温度：%d  主板温度:%d、功放板温度：%d\n",(uint8_t)gs_WidgetTempValInfo.TempVal3, (uint8_t)gs_WidgetTempValInfo.TempVal1, (uint8_t)gs_WidgetTempValInfo.TempVal2 ); 
            snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "喇叭温度：%d\n",(uint8_t)gs_WidgetTempValInfo.TempVal3); 

            //add version and sn
            snprintf(cziMessage + strlen(cziMessage), 255, "固件版本：%s\n", software_version);
            snprintf(cziMessage + strlen(cziMessage), 255, "SN: %s\n", sn);

            // if(!versionFlag){
            //     snprintf(cziMessage + strlen(cziMessage), 255, "非最新版本，请更新！\n");
            // }
        }else {
            // monitor
            char str[MAX_STR_LENGTH];
            Widget_GenerateString(str, gs_RecordValue);
            snprintf(cziMessage, 255, "monitor:%s\n", str);               

            

            snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "music(%d/%d):%s\n", gWidgetCziMediaInfo.musicListIndex, gWidgetCziMediaInfo.sum, gWidgetCziMediaInfo.musicName);

            
            if (detectFlag){
                snprintf(cziMessage + strlen(cziMessage), 255, "%s\n", "AI recognizes the current scene as personnel detection");
            }

            // snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "Alarm Lamp:%s\n", aLightListInfo_En[gWidgetCziMediaInfo.aLightSwitch].txtList);   
            snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "Gimbal Light:%s\n", aLightListInfo_En[gWidgetCziMediaInfo.aLarmSwitch].txtList); 
            snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "Volume:%d\n", gWidgetCziMediaInfo.volume);  

            snprintf(cziMessage + strlen(cziMessage), MSG_MAX_LEN, "buge temperature:%d\n",(uint8_t)gs_WidgetTempValInfo.TempVal3); 

            //add version and sn
            snprintf(cziMessage + strlen(cziMessage), 255, "software_version:%s\n", software_version);
            snprintf(cziMessage + strlen(cziMessage), 255, "SN: %s\n", sn);

            // if(!versionFlag){
            // snprintf(cziMessage + strlen(cziMessage), 255, "Not the latest version, please update\n");
            // }
        }

        djiStat = DjiWidgetFloatingWindow_ShowMessage(cziMessage);
        if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Floating window show message error, stat = 0x%08llX", djiStat);
        }

        osalHandler->TaskSleepMs(200);
    }
}


#ifndef __CC_ARM
#pragma GCC diagnostic pop
#endif


T_DjiReturnCode DjiTestWidget_SetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t value,
                                                    void *userData)
{
    // USER_UTIL_UNUSED(userData);

    USER_LOG_INFO("Set widget value, widgetType = %s, widgetIndex = %d ,widgetValue = %d",
                  s_widgetTypeNameArray[widgetType], index, value);
    s_widgetValueList[index] = value;

    if (userData == NULL) {
        if(gs_widgetCziServiceHandler != NULL) 
            gs_widgetCziServiceHandler(widgetType, index, value);
    }
    

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode DjiTestWidget_GetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t *value,
                                                    void *userData)
{
    USER_UTIL_UNUSED(userData);
    USER_UTIL_UNUSED(widgetType);

    *value = s_widgetValueList[index];

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode Widget_MediaListInfo(T_WidgetCziMediaInfo *WidgetCziMediaInfo)
{
    gWidgetCziMediaInfo.musicListIndex = WidgetCziMediaInfo->musicListIndex;
    memset(gWidgetCziMediaInfo.musicName, 0x00, sizeof(WidgetCziMediaInfo->musicName));
    memcpy(gWidgetCziMediaInfo.musicName, WidgetCziMediaInfo->musicName, sizeof(WidgetCziMediaInfo->musicName));
    gWidgetCziMediaInfo.sum = WidgetCziMediaInfo->sum;
    
    gWidgetCziMediaInfo.mediaMode = WidgetCziMediaInfo->mediaMode;
    gWidgetCziMediaInfo.mediaStats = WidgetCziMediaInfo->mediaStats;
    gWidgetCziMediaInfo.volume = WidgetCziMediaInfo->volume;

    gWidgetCziMediaInfo.aLightSwitch = WidgetCziMediaInfo->aLightSwitch;
    // gWidgetCziMediaInfo.gLightBright = WidgetCziMediaInfo->gLightBright;
    // gWidgetCziMediaInfo.gLightSwitch = WidgetCziMediaInfo->gLightSwitch;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


/* set widget index value */
void CziWidget_SetIndexValue(uint32_t index, int32_t value)
{
    USER_LOG_INFO("11111111Set widget value, widgetIndex = %d ,widgetValue = %d\n", index, value);
    s_widgetValueList[index] = value;
}
void CziWidget_GetIndexValue(uint32_t index, int32_t *value)
{
    *value = s_widgetValueList[index];
}

void CziWidget_SetLightSwitch(int light_code)   
{
    printf("111111111111111  light_code:%d   \n", light_code);
    // if (light_switch == 0xA1) 
    // {
    //     CziWidget_SetIndexValue(WIDGET_INDEX_FLASH_SWITCH, 0x00);
    // }else if (light_switch == 0xA0)
    // {
    //     CziWidget_SetIndexValue(WIDGET_INDEX_FLASH_SWITCH, light_code + 0x01);
    //     // CziWidget_SetLightCode(light_code);
    // }
    //  else 
    // {
    //     USER_LOG_ERROR("Invalid light switch value: %d", light_switch);
    // }
    CziWidget_SetIndexValue(WIDGET_INDEX_FLASH_SWITCH, light_code - 1);
}

void CziWidget_SetLightCode(int light_code)
{
    if (light_code >= 0x00 && light_code <= 0x02) 
    {
        CziWidget_SetIndexValue(WIDGET_INDEX_FLASH_SWITCH, light_code + 0x01);
    } else 
    {
        USER_LOG_ERROR("Invalid light code value: %d\n", light_code);
    }
}

void CziWidget_SetLoopModel(int loop_model)
{
    printf("loop_model is %d\n",loop_model);
    CziWidget_SetIndexValue(WIDGET_INDEX_MUSIC_SINGLE_CYCLE, loop_model - 0xc0);
}

void CziWidget_SetStartupMute(int startup_mute)
{   
    printf("startup_mute is %x\n",startup_mute);
    CziWidget_SetIndexValue(WIDGET_INDEX_VERSION_BROADCAST, startup_mute - 0xf0);
}




void CziWidget_SendMessage(char* value, char* widget_flag){
    if(strcmp(widget_flag,"sn") == 0){
        strcpy(sn,value);
    }
    else if(strcmp(widget_flag, "software_version") == 0){
        strcpy(software_version,value);
    }
    else{
        printf("error!!!\n");
    }
}

void CziWidget_GetRecordAmp(unsigned char recordvalue)
{
    gs_RecordValue = recordvalue;
}
/****************** (C) COPYRIGHT DJI Innovations *****END OF FILE****/
// void CziWidget_SetAiVideoStream(int AiVideoState) {
//     if (AiVideoState == 0xF1){
//         CziWidget_SetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, 0x01);
//     }
//     else if (AiVideoState == 0xF0 || AiVideoState == 0xF2 || AiVideoState == 0xF3){
//         CziWidget_SetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, 0x00);
//     }
// }
void UpdateAiDetectResult(int* aiDetectResult) {
    char filePath[] = FILEPATH_AI_DETECT_STATE;
    struct stat fileStat;
    
    if (stat(filePath, &fileStat) != 0) {
        USER_LOG_ERROR("Cannot access file.");
        return;
    }

    if (CziJsonHandler_Open(filePath, 0) != 0) {
        USER_LOG_ERROR("Cannot open JSON file.");
        return;
    }

    int result;
    if (CziJsonHandler_ReadInt(JSON_KEY_AI_DETECT_RESULT, &result) != 0) {
        USER_LOG_ERROR("Invalid JSON format or result not found.");
        CziJsonHandler_Close(0);
        return;
    }

    *aiDetectResult = (result == 0) ? 1 : 0;
    CziJsonHandler_Close(0);
}

void CziWidget_UpdateMainTempVal(float Val1, float Val2)
{
    gs_WidgetTempValInfo.TempVal1 = Val1;
    gs_WidgetTempValInfo.TempVal2 = Val2;
}
void CziWidget_UpdateBugleTempVal(float Val3)
{
    gs_WidgetTempValInfo.TempVal3 = Val3;
}
/****************** (C) COPYRIGHT DJI Innovations *****END OF FILE****/
