/**
 ********************************************************************
 * @author yealkio
 * @file    protocol_shortFormatHandler.h
 * @version V1.0.0
 * @date    2024/01/01
 * @brief   This file is to indicate external communication with devices.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */
#ifndef _PROTOCOL_SHORTFORMATHANDLER_H_
#define _PROTOCOL_SHORTFORMATHANDLER_H_

#include "protocol_commonHandler.h"
#define PROT_SHORT_FORMAT_LENGTH_EXCEPT 4

int Prot_ShortFormatUnpack(unsigned char *data, int len, T_ProtRspData *output);
int Prot_ShortFormatPack(E_ProtocolCommandAll protCmd, unsigned char *data, short int size, T_ProtRspData *rsp);
int Prot_ShortFormatAntiStickedPackets(T_ProtCommonWorkSpace *work, unsigned char *data, int len, unsigned char *reply, int *replyLen);
#endif /* _PROTOCOL_SHORTFORMATHANDLER_H_ */
