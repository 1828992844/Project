/**
 ********************************************************************
 * @author yealkio
 * @file    protocol_longFormatHdl.h
 * @version V1.0.0
 * @date    2024/01/01
 * @brief   This file is to indicate external communication with devices.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#ifndef _PROTOCOL_LONG_FORMAT_HDL_H_
#define _PROTOCOL_LONG_FORMAT_HDL_H_

#include "protocol_commonHandler.h"

#define PROT_LONG_FORMAT_LENGTH_EXCEPT 6

int Prot_LongFormatUnpack(unsigned char *input_data, int len, T_ProtRspData *output);
int Prot_LongFormatPack(E_ProtocolCommandAll protCmd, unsigned char *input_data, short int size, T_ProtRspData *rsp);
int Prot_LongFormatAntiStickedPackets(T_ProtCommonWorkSpace *work, unsigned char *input_data, int len, unsigned char *reply, int *replyLen);
#endif /* _PROTOCOL_LONG_FORMAT_HDL_H_ */
