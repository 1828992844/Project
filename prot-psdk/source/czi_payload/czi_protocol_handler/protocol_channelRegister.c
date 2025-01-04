/**
 ********************************************************************
 * @author yealkio
 * @file    protocol_channelRegister.c
 * @version V1.0.0
 * @date    2024/01/01
 * @brief   This file is to indicate external communication with devices.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "protocol_channelRegister.h"
#include "protocol_commonHandler.h"

int Prot_ChannelRegister(T_ProtChannelConfig *channalCfg, T_ProtChannelHdl *channalHdl)
{
    channalHdl->protDataChannal = channalCfg->channelNum;
    channalHdl->workSpace =  (T_ProtChannelWorkSpace *)malloc(sizeof(T_ProtChannelWorkSpace));
    return 0;
}

int CziProt_ChannelUnregister(T_ProtChannelHdl *channalHdl)
{
    return 0;
}
