/**
 ********************************************************************
 * @author  CYN 
 * @file    czi_logClient.c
 * @version V0.0.0
 * @date    2024/4/8
 * @brief   This file is to indicate client log.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#include "czi_log.h"
#include "czi_mailbox.h"
#include "czi_config.h"
#include "czi_packet.h"
#include "czi_logClient.h"

int Czilog_ClientInit(T_CziLogUserFormat *userFmt)
{
    int err = -1;
    char addLogIndex[10] = "";
    T_CziMailboxComms cm;

    if (userFmt == NULL) {
        printf("userFmt is NULL\n");
        return -1;
    }
    if (CZI_USER_LOG_MODE == SYS_ELOG_MODE || CZI_USER_LOG_MODE == SYS_BOTH_MODE) {
        sprintf(addLogIndex, "%04d", Log_GetLogIndex(CZI_USER_LOG_LOCATION));    
        CziElog_LogInit(userFmt->logServerLocationPath, addLogIndex);
    } else if (CZI_USER_LOG_MODE == SYS_MLOG_MODE || CZI_USER_LOG_MODE == SYS_BOTH_MODE) {
        err = CziMailbox_InitTarget(&cm, MAILBOX_COMMS_HANDLER, MAILBOX_COMMS_PORT);
        if (-1 == err)
            return -1;
    }
}

int Czilog_SendData(char *data, int len)
{
    return CziMailbox_SendData(MAILBOX_COMMS_HANDLER, data, len);
}
