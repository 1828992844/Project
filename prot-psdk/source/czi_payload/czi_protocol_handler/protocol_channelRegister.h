/**
 ********************************************************************
 * @author yealkio
 * @file   protocol_channelRegister.c
 * @version V1.0.0
 * @date    2024/01/01
 * @brief   This file is to indicate external communication with devices.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#ifndef _PROTOCOL_CHANNELREGISTER_H_
#define _PROTOCOL_CHANNELREGISTER_H_

#include "protocol_commonHandler.h"

typedef T_ProtCommonWorkSpace T_ProtChannelWorkSpace;

/**
 * @brief The protocol channal configuration parameters.
 */
typedef struct {
    int channelNum;                         //data anti-stick handle channal
    E_Protocol_Type type;                   //data handler type.
} T_ProtChannelConfig;

/**
 * @brief The protocol channal configuration handler for protocol function.
 */
typedef struct {
    int protDataChannal;
    T_ProtChannelWorkSpace *workSpace;
    void *userData;
} T_ProtChannelHdl;

int Prot_ChannelRegister(T_ProtChannelConfig *channalCfg, T_ProtChannelHdl *channalHdl);
int Prot_ChannelUnRegister(T_ProtChannelHdl *channalHdl);

#undef PACKET_MAX_LEN
#undef LEN_CMD_MAX
#endif /* _PROTOCOL_CHANNELREGISTER_H_ */
