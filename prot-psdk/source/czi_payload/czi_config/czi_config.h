
/**
 ********************************************************************
 * @author roocket
 * @file    config.h
 * @version V0.0.0
 * @date    2024/3/138
 * @brief   This file is to indicate configuration.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "czi_log.h"
// #include <pthread.h>

/* USER REFERENCE */

typedef enum
{
	USE_SERVER_TYPE = 0,
	USE_CLIENT_TYPE
}E_CZI_USER_TYPE;

typedef enum
{
	USE_MLOG_MODE = 0,
	USE_ELOG_MODE,
	USE_BOTH_MODE
}E_CZI_LOG_MODE;

/* USER REFERENCE */

/* USERS fills The type and mode */
#define CZI_USER_LOG_MODE	USE_MLOG_MODE			/*CZI_USER_LOG_MODE only support 0、1 and 2*/

/* USERS fills log information */
//#define CZI_USER_LOG_LOCATION "../czi_log"
#define CZI_USER_LOG_LOCATION 				"/czzn/log/DJI"
#define LOG_FILE_START_NAME					"czi_server"

#define CZI_USER_LOG_FORMAT 				">><<"
#define CZI_USER_LOG_TAG 					"DJI"
/* write to the log file level */
#define ELOG_LVL_WRITE_FILE               ELOG_LVL_DEBUG 

/* USERS fills the mailbox information */

/* server */
/* The index for mailbox like 0,1,2... */
#define MAILBOX_COMMS_HANDLER 12

/* The port for mailbox like 8306(base) */
#define MAILBOX_COMMS_PORT 8318

#endif /* _CONFIG_H_ */
