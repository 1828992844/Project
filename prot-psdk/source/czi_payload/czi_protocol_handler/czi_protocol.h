
/**
 ********************************************************************
 * @author yealkio
 * @file    czi_protocol.h
 * @version V1.0.0
 * @date    2024/01/01
 * @brief   This file is to indicate external communication with devices.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */

#ifndef _CZI_PROTOCOL_H_
#define _CZI_PROTOCOL_H_

#include "protocol_commonHandler.h"
#include "protocol_channelRegister.h"

#define ENABLE_CHECK_CMD_STATUS (0)

#define PROT_LONG_FORMAT_DEFAULT_PARAM(chn, func) {\
        .protDataChannel = chn,                   \
        .type = PROTOCOL_LONG_FORMAT,           \
        .sendCallback = func, \
}

#define PROT_SHORT_FORMAT_DEFAULT_PARAM(chn,func) {\
        .protDataChannel = chn,                   \
        .type = PROTOCOL_SHORT_FORMAT,           \
        .sendCallback = func, \
}


/**
 * @brief Configuration for protocol's parameters.
 * @param protDataChannel Channel id.
 * @param type  The protocol type which data need to handle with.
 * @param sendCallback Pointer of sender callback function.
 * @param userData Reserved.
 */
typedef struct {
    int protDataChannel;                    /* protocol channel */
    E_Protocol_Type type;                   /* data handler type */
    pProtocolResponseFunc sendCallback;     /* sender callback function */
    void *userData;                         /* reserved */
} T_CziProtConfiguration;

/**
 * @brief Handler of protocol.
 * @param type  The protocol type which data need to handle with.
 * @param sendCallback Pointer of sender callback function.
 * @param channelHdl Channel handler, include workspace
 * @param userData Reserved.
 */
typedef struct {
    E_Protocol_Type type;
    pProtocolResponseFunc sendCallback;     /* sender callback function */
    T_ProtChannelHdl *channelHdl;           /* channel handler */
    T_ProtRspData rsp;
    void *userData;
} T_CziProtHandler;


/**
 * @brief Init prot handler with specify cfg
 * @param protCfg The configuration about prot handler.
 * @param protHdl The prot handler, must be declare as global variable.
 * @return 0 means success, other fail!

*/
int CziProt_ConfigurationInit(T_CziProtConfiguration *protCfg,T_CziProtHandler *protHdl); 

/**
 * @brief Deinit prot handler.
 * @return 0 means success, other fail!

*/
int CziProt_HandlerDeInit(T_CziProtHandler *protHdl);

/**
 * @brief Analyze the data and execute.
 * @param protHdl The pointer of the prot has been initialized, must be declare as global variable.
 * @param raw_data The pointer of input data need to analyze.
 * @param len The lenth of input data.
 * @return 0 means success, other fail!

*/
int CziProt_UserDataHdl(T_CziProtHandler *protHdl, unsigned char *raw_data, int len);

/**
 * @brief Just nalyze the code and unpack.
 * @param protHdl The pointer of the prot has been initialized, must be declare as global variable.
 * @param raw_data The pointer of input data need to analyze.
 * @param len The lenth of input data.
 * @return 0 means success, other fail!

*/
int CziProt_DataUnpackHdl(T_CziProtHandler *protHdl, unsigned char *raw_data, int len);

extern int Prot_CommonPacketHandler(E_Protocol_Type type, E_ProtocolCommandAll protCmd, unsigned char *data, short int size, T_ProtRspData *rsp);
extern bool CziProt_CmdCheckValue(unsigned char cmd);
extern int CziProt_CheckCmdIsVaildType(int cmd);
#endif /* _CZI_PROTOCOL_H_ */
