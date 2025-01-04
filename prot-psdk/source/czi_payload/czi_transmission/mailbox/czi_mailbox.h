/**
 ********************************************************************
 * @author roocket
 * @file    czi_mailbox.h
 * @version V0.0.0
 * @date    2023/5/12
 * @brief   This file is to indicate mailbox for communication with.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#ifndef __CZI_MAILBOX_H_
#define __CZI_MAILBOX_H_
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
 * @brief To define the format of mailbox's transmission.
 */
#define MAILBOX_MAX_NUMBER 100
#define MAILBOX_MAX_LENGTH (1300)
#define SERVER_ADDR "127.0.0.1"

/* definition for ports at max 100*/
#define MAILBOX_COMMS_BASE 8306
#define MAILBOX_COMMS_PSDK (MAILBOX_COMMS_BASE + 1)
#define MAILBOX_COMMS_PROT (MAILBOX_COMMS_BASE + 14) //+14

/* definiton for id at max 100 */
#define MAILBOX_COMMS_HANDLER_BASE 0
#define MAILBOX_COMMS_HANDLER_PSDK (MAILBOX_COMMS_HANDLER_BASE + 0)
#define MAILBOX_COMMS_HANDLER_PROT (MAILBOX_COMMS_HANDLER_BASE + 14) //+14

typedef struct __mailbox_addr
{
    int fd;                  /* file descriptor of mailbox */
    unsigned short port;     /* port of mailbox */
    struct sockaddr_in addr; /* address of ip */
} T_CziMailboxComms;

/**
 * @brief Init owner/original for mailbox.
 * @note For setting up the communication with other clients.
 * @param comms The communication handler.
 * @param id The mailbox identity.
 * @param port The port of mailbox.
 * @return Executed result.
 */
int CziMailbox_InitOriginal(T_CziMailboxComms *comms, unsigned short id, unsigned short port);

/**
 * @brief Init target for mailbox.
 * @note For setting up the communication with original mailbox.
 * @param comms The communication handler.
 * @param id The mailbox identity.
 * @param port The port of mailbox.
 * @return Executed result.
 */
int CziMailbox_InitTarget(T_CziMailboxComms *comms, unsigned short id, unsigned short port);

/**
 * @brief Recv data to mailbox.
 * @note For setting up the communication with specific mailbox.
 * @param id The mailbox identity.
 * @param data The data transfered to mailbox.
 * @param len[output] The length of data.
 * @return Executed result.
 */
short CziMailbox_RecvData(unsigned short mb, char *data, int *len);

/**
 * @brief Send data to mailbox.
 * @note For setting up the communication with specific mailbox.
 * @param id The mailbox identity.
 * @param data The data transfered to mailbox.
 * @param len The length of data.
 * @return Executed result.
 */
int CziMailbox_SendData(unsigned short mb, char *data, int len);


#endif /* __CZI_MAILBOX_H_ */
