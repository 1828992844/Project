/**
 ********************************************************************
 * @author roocket
 * @file    czi_packet.h
 * @version V0.0.0
 * @date    2023/5/18
 * @brief   This file is to indicate mailbox for packets.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "czi_packet.h"

int CziPacket_PackMailboxMsg(unsigned char whichMail, char *data, int len, T_CziMailboxPacket *mp) 
{
    mp->mailboxId = whichMail;
    memcpy(mp->data, data, len);
    mp->length = len + sizeof(whichMail);
    return 0;
}

int CziPacket_UnpackMailboxMsg(unsigned char *mailboxMsg, int len, T_CziMailboxPacket *mp) 
{
    if (len < 0)
        return -1;

    mp->mailboxId = mailboxMsg[0];
    memcpy(mp->data, mailboxMsg + 1, len - sizeof(unsigned char));
    mp->length = len;  
    return 0;
}
