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

#ifndef __CZI_PACKET_H_
#define __CZI_PACKET_H_
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>

/**
 * @brief To define the packet about mailbox.
 * like 
 * mailboxId   data  length
 *     1       len     4
 */
#define MAILBOX_PACKET_MAX_LENGTH (1300)
typedef struct __mailbox_packet
{
    unsigned char mailboxId;
    unsigned char data[MAILBOX_PACKET_MAX_LENGTH];
    int length;
} T_CziMailboxPacket;

/**
 * @brief Pack the packet for mailbox.
 * @note Based on the mailbox's packet, it transfers the stable-format for mailbox.
 * @param whichMail The mail from whom.
 * @param data The handling data for this mailbox.
 * @param len The length.
 * @param mp The packet.
 * @return Executed result.
 */
int CziPacket_PackMailboxMsg(unsigned char whichMail, char *data, int len, T_CziMailboxPacket *mp);

/**
 * @brief Unpack the packet for mailbox.
 * @note Based on the mailbox's packet, it transfers the stable-format for mailbox.
 * @param mailboxMsg The message from mailbox. 
 * @param len The message length.
 * @param mp The packet.
 * @return Executed result.
 */
int CziPacket_UnpackMailboxMsg(unsigned char *mailboxMsg, int len, T_CziMailboxPacket *mp);

#endif /* __CZI_PACKET_H_ */
