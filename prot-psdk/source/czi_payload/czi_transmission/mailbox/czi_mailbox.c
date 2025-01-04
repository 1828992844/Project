/**
 ********************************************************************
 * @author roocket
 * @file    czi_mailbox.c
 * @version V0.0.0
 * @date    2023/5/12
 * @brief   This file is to indicate mailbox for communication with.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#include "czi_mailbox.h"
#include "czi_packet.h"

T_CziMailboxComms mbLists[MAILBOX_MAX_NUMBER];
unsigned char owner;
int CziMailbox_InitOriginal(T_CziMailboxComms *comms, unsigned short id, unsigned short port)
{
    int fd;
    int ret;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == fd)
        return -1;

    comms->addr.sin_family = AF_INET;
    comms->addr.sin_port   = htons(port);
    comms->addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(fd,
              (struct sockaddr*)&comms->addr,
              sizeof(struct sockaddr_in));
    if (0 > ret)
        return -1;

    comms->fd   = fd;
    comms->port = port;

    /* copied to users */
    mbLists[id].fd = fd;
    mbLists[id].port = port;
    mbLists[id].addr = comms->addr;
    owner = id;

    return 0;
}

int CziMailbox_InitTarget(T_CziMailboxComms *comms, unsigned short id, unsigned short port)
{
    int fd;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == fd)
        return -1;

    comms->addr.sin_family = AF_INET;
    comms->addr.sin_port   = htons(port);
    comms->addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    comms->fd   = fd;
    comms->port = port;

    /* copied to users */
    mbLists[id].fd = fd;
    mbLists[id].port = port;
    mbLists[id].addr = comms->addr;

    return 0;  
}

short CziMailbox_RecvData(unsigned short mb, char *data, int *len)
{
    int errCode = -1;
    int recvLen = -1;
    char buffer[MAILBOX_MAX_LENGTH] = "";
    struct sockaddr_in srcAddr = {};
    socklen_t addrLen = sizeof(struct sockaddr_in);
    T_CziMailboxPacket mp = {0};

    recvLen = recvfrom(mbLists[mb].fd,
            buffer,
            MAILBOX_MAX_LENGTH,
            0,
            (struct sockaddr*)&srcAddr,
            &addrLen);
    if (-1 == recvLen)
        return -1;
    errCode = CziPacket_UnpackMailboxMsg(buffer, recvLen, &mp);
    if (-1 == errCode)
        return -1;
    memcpy(data, mp.data, recvLen - sizeof(unsigned char));

    *len = recvLen - sizeof(unsigned char);
    return mp.mailboxId;
}

int CziMailbox_SendData(unsigned short mb, char *data, int len)
{
    int errCode = -1;
    fd_set fdListen;
    int maxFd;
    struct timeval tm;
    T_CziMailboxPacket mp = {0};

    maxFd = mbLists[mb].fd;
    FD_ZERO(&fdListen);
    FD_SET(mbLists[mb].fd, &fdListen);

    tm.tv_sec  = 0;
    tm.tv_usec = 300;
    errCode = select(maxFd + 1, &fdListen, NULL, NULL, &tm);

    switch (errCode) {
    case -1:
        return -1;
    default:

        /* pack the packet */
        errCode = CziPacket_PackMailboxMsg(owner, data, len, &mp);
        if (-1 == errCode)
            return -1;

        errCode = sendto(mbLists[mb].fd,
                &mp.mailboxId,
                mp.length,
                0,
                (struct sockaddr*) &mbLists[mb].addr,
                sizeof(struct sockaddr_in));
        if (-1 == errCode)
            return -1;
        break;
    }
    return 0; 
}
