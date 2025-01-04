/**
 ********************************************************************
 * @author CYN
 * @file    czi_logClient.h
 * @version V0.0.0
 * @date    2024/4/8
 * @brief   This file is to indicate server log.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#ifndef _CZI_LOG_CLIENT_H_
#define _CZI_LOG_CLIENT_H_
#include "czi_log.h"

/**
 * @brief czi log client init.
 * @param userFmt userFmt user format information..
 * @return Executed result.
 */
int Czilog_ClientInit(T_CziLogUserFormat *userFmt);
/**
 * @brief czi log sever init.
 * @param userFmt userFmt user format information..
 * @return Executed result.
 */
int Czilog_SendData(char *data, int len);

#endif

