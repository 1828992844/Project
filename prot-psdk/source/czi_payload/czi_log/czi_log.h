/**
 ********************************************************************
 * @author CYN
 * @file    czi_log.h
 * @version V0.0.0
 * @date    2024/3/8
 * @brief   This file is to indicate log.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */
#ifndef _CZI_LOG_H_
#define _CZI_LOG_H_
#include "../czi_elog/inc/elog.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* configuration from users */
#include "czi_config.h"

/**
 * @note no log name but its location 
 */
#define MAX_LOG_NUM			20	
#define LOG_NAME_BUFF_LEN	512
#define LOG_MALLOC_LEN		128 
#define LOG_INDEX_STR_LEN	5

/* color for log level as macro definitions */
#define MLOG_LEVEL_COLOR_MAGENTA      "\033[30m"
#define MLOG_LEVEL_COLOR_RED        "\033[31m"
#define MLOG_LEVEL_COLOR_GREEN      "\033[32m"
#define MLOG_LEVEL_COLOR_YELLOW     "\033[33m"
#define MLOG_LEVEL_COLOR_BLUE       "\033[34m"
#define MLOG_LEVEL_COLOR_PURPLE     "\033[35m"
#define MLOG_LEVEL_COLOR_DARKGREEN  "\033[36m"
#define MLOG_LEVEL_COLOR_WHITE      "\033[37m"

/* log level calling areas */
#define MLOG_LOG_ASSERT_LEVEL  MLOG_LEVEL_COLOR_MAGENTA
#define MLOG_LOG_ERROR_LEVEL   MLOG_LEVEL_COLOR_RED
#define MLOG_LOG_WARNING_LEVEL MLOG_LEVEL_COLOR_YELLOW
#define MLOG_LOG_NOTICE_LEVEL  MLOG_LEVEL_COLOR_DARKGREEN
#define MLOG_LOG_INFO_LEVEL    MLOG_LEVEL_COLOR_BLUE
#define MLOG_LOG_DEBUG_LEVEL   MLOG_LEVEL_COLOR_PURPLE
#define MLOG_LOG_VERBOSE_LEVEL MLOG_LEVEL_COLOR_WHITE

#define filename(x) strrchr(x,'/') ? strrchr(x,'/') + 1 : x
#define TAG_A "Assert"
#define TAG_E "Error"
#define TAG_W "Warning"
#define TAG_I "Info"
#define TAG_D "Debug"
#define TAG_V "Verbose"
#define TAG_N "Notice"

#define clog_a(fmt, ...) \
		CziCommlog_BaseRecord(MLOG_LOG_ASSERT_LEVEL,	TAG_A , "[%s-%s:%d)" fmt, filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__) 
#define clog_e(fmt, ...) \
		CziCommlog_BaseRecord(MLOG_LOG_ERROR_LEVEL, 	TAG_E , "[%s-%s:%d)" fmt, filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define clog_w(fmt, ...) \
		CziCommlog_BaseRecord(MLOG_LOG_WARNING_LEVEL,	TAG_W , "[%s-%s:%d)" fmt, filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define clog_i(fmt, ...) \
		CziCommlog_BaseRecord(MLOG_LOG_INFO_LEVEL,		TAG_I , "[%s-%s:%d)" fmt, filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define clog_d(fmt, ...) \
		CziCommlog_BaseRecord(MLOG_LOG_DEBUG_LEVEL,		TAG_D , "[%s-%s:%d)" fmt, filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__) 
#define clog_v(fmt, ...) \
		CziCommlog_BaseRecord(MLOG_LOG_VERBOSE_LEVEL,	TAG_V , "[%s-%s:%d)" fmt, filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__) 
#define clog_n(fmt, ...) \
		CziCommlog_BaseRecord(MLOG_LOG_NOTICE_LEVEL, 	TAG_N , "[%s-%s:%d)" fmt, filename(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__) 

#define CziLog_Assert   clog_a
#define CziLog_Error    clog_e
#define CziLog_Warning  clog_w
#define CziLog_Info     clog_i
#define CziLog_Debug    clog_d
#define CziLog_Verbose  clog_v
#define CziLog_Notice	clog_n

#define CZI_LOG_LOCATION(EXP)                                  \
	do {                                                  \
		if (EXP[strlen(EXP) - 1] == '/') {               \
			snprintf(logPath, sizeof(logPath), "%s%s", EXP, LOG_FILE_START_NAME); \
		} else {                                          \
			snprintf(logPath, sizeof(logPath), "%s/%s", EXP, LOG_FILE_START_NAME); \
		}                                                 \
	} while(0);

/**
 * @brief Definitions for formats of log server.
 */
typedef struct _log_server_format_
{
    char logServerLocationPath[256];
    char logTag[256];
    char logFormat[256];
    unsigned char logServerChannel;
    unsigned char logServerChannelId;
} T_CziLogUserFormat;


/* SYSTEM REFERENCE */
typedef enum
{
	SYS_SERVER = 0,
	SYS_CLIENTE
}E_CZI_LOG_SYS_TYPE;

typedef enum
{
	SYS_MLOG_MODE = 0,
	SYS_ELOG_MODE,
	SYS_BOTH_MODE
}E_CZI_LOG_SYS_MODE;
/*************/

/**
 * @brief czi log init.
 * @param userFmt userFmt user format information..
 * @return Executed result.
 */
int CziLog_Init(T_CziLogUserFormat *userFmt);
/**
 * @brief czi common log bese record.
 * @param level log level.
 * @param tag user log thread tag.
 * @param format user log format.
 * @return Executed result.
 */
int CziCommlog_BaseRecord(const char *level, const char *tag, char *format, ...);

void CziElog_LogInit(const char *log_name, const char *index);

unsigned int Log_GetLogIndex(char *combinedFilePath);

#endif /* _CZI_LOG_H_ */
