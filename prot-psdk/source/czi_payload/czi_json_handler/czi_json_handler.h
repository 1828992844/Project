
/**
 ********************************************************************
 * @author roocket
 * @file    CZI_JSON_HANDLER.h
 * @version V0.0.0
 * @date    2024/3/138
 * @brief   This file is to indicate configuration.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#ifndef _CZI_JSON_HANDLER_H_
#define _CZI_JSON_HANDLER_H_


#include <pthread.h>
#include "./cJSON/czi_JSON.h"


char CziJsonHandler_Close(char isWrite);
char CziJsonHandler_Open(char* fName, char isClear);
char* CziJsonHandler_ReadString(char* key);
char* CziJsonHandler_Read2String(char* key, char* key2);
int CziJsonHandler_ReadInt(char* key, int* value);
int CziJsonHandler_Read2Int(char* key, char* key2, int* value);
int CziJsonHandler_WriteInt(char* key, int value);
int CziJsonHandler_WriteString(char* key, char* value);
int CziJsonHandler_Write2String(char* key, char* key2, char* value);
char CziJsonHandler_GetRootString(char *outBuf, int bufLen);
czi_JSON* CziJsonHandler_GetRoot();

#endif /* _CZI_JSON_HANDLER_H_ */