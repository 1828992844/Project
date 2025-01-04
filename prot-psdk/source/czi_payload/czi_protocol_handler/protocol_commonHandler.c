/**
 ********************************************************************
 * @author yealkio
 * @file    protocol_commonHandler.c
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

#include "protocol_commonHandler.h"
#include "protocol_longFormatHandler.h"
#include "protocol_shortFormatHandler.h"

static unsigned char prot_cmd_status[256] = {PROT_HAVE_NO_WORK};

unsigned char Prot_ValidateXOR(unsigned char *data, short int size)
{
    unsigned char result = 0;
    int i;

    for ( i = 0; i < size; i++ )
    {
        result ^= data[i];
    }
    
    return result;
}

bool CziProt_CmdCheckValue(unsigned char cmd)
{
    
    switch(cmd){
        // #define X_MACRO(a, b) case a:
        #define X_CMD_COMMON_MACRO(a, b) case a:
        #define X_CMD_FILE_MACRO(a, b) case a:
        #define X_CMD_MEDIA_MACRO(a, b) case a:
        #define X_CMD_GL_MACRO(a, b) case a:
        #define X_CMD_GPI_MACRO(a, b) case a:

        __PACKET_CMD

        // #undef X_MACRO
        #undef X_CMD_COMMON_MACRO
        #undef X_CMD_FILE_MACRO
        #undef X_CMD_MEDIA_MACRO
        #undef X_CMD_GL_MACRO
        #undef X_CMD_GPI_MACRO
        
            return true;
        default:
            return false;
    }
}

int Prot_CommonPacketHandler(E_Protocol_Type type, E_ProtocolCommandAll protCmd, unsigned char *data, short int size, T_ProtRspData *rsp)
{
    int realLen = -1;
    switch (type) {
        case PROTOCOL_SHORT_FORMAT:
        {
            realLen = Prot_ShortFormatPack(protCmd, data, size, rsp);
            break;
        }
            break;
        case PROTOCOL_LONG_FORMAT:
        {
            realLen = Prot_LongFormatPack(protCmd, data, size, rsp);
            break;
        }
        default:
            printf("wrong protocol type %d  line:%d\r\n", type, __LINE__);
            break;
    }
    return realLen;
}

int Prot_CommonHdl(E_Protocol_Type type, unsigned char *data, int len, T_ProtRspData *output)
{
    int errCode = -1;
    switch (type) {
        case PROTOCOL_SHORT_FORMAT:
            break;
        case PROTOCOL_LONG_FORMAT:
        {
            errCode = Prot_LongFormatUnpack(data, len, output);
            break;
        }
        default:
            printf("wrong protocol type %d  line:%d\r\n", type, __LINE__);
            break;
    }
    return errCode;
}


int CziInternal_AntiStickedPackets(T_ProtCommonWorkSpace *work, unsigned char *data, int len, unsigned char *reply, int *replyLen)
{
    // Proto_addData(work, data, len);
    // while (Proto_ValidatePacket(work, reply, replyLen) >= 0)
    //     return 0;
    // return -1;
}


int CziProt_CheckCmdIsVaildType(int cmd)
{
    #define X_CMD_COMMON_MACRO(a, b) if(a==cmd)\
                                    return CMD_TYPE_COM;

    #define X_CMD_MEDIA_MACRO(a, b) if(a==cmd)\
                                    return CMD_TYPE_MEDIA;

    #define X_CMD_FILE_MACRO(a, b) if(a==cmd)\
                                    return CMD_TYPE_FILE;

    #define X_CMD_GL_MACRO(a, b) if(a==cmd)\
                                    return CMD_TYPE_GL;

    #define X_CMD_GPI_MACRO(a, b) if(a==cmd)\
                                    return CMD_TYPE_GPI;

    __PACKET_CMD

    #undef X_CMD_COMMON_MACRO
    #undef X_CMD_FILE_MACRO
    #undef X_CMD_MEDIA_MACRO
    #undef X_CMD_GL_MACRO
    #undef X_CMD_GPI_MACRO

    return -1;

}


/* Whether the command contains the inner command like F2/F3 */
static unsigned char Prot_ContainsInnerCmd(E_ProtocolCommandAll cmd, unsigned char inner_cmd)
{
    unsigned char inner = 0x00;

    switch (cmd) {
        case CZI_PROTOCOL_COMMON_CMD_STREAM_DOWN:
        case CZI_PROTOCOL_COMMON_CMD_STREAM_UP:
            inner = inner_cmd;
            break;
        default:
            break;
    }
    return inner;
}


void Prot_SetRspStatus(E_ProtocolCommandAll cmd, unsigned char inner_cmd, E_ProtRspStatus set_status)
{
    unsigned char inner = 0x00;

    inner = Prot_ContainsInnerCmd(cmd, inner_cmd);
    if (inner) {
        // ESP_LOGI(PROTOCOL_USF_HANDLER_TAG, "found the inner cmd");
        prot_cmd_status[cmd] = inner;
    }
    else{
        // ESP_LOGI(PROTOCOL_USF_HANDLER_TAG,"It has set status for %02x",cmd);
        prot_cmd_status[cmd] = set_status;
    }

}

int Prot_CheckRspStatus(E_ProtocolCommandAll cmd, unsigned char inner_cmd)
{
    unsigned char inner = 0x00;

    inner = Prot_ContainsInnerCmd(cmd, inner_cmd);
    if (inner) {
        if(prot_cmd_status[cmd] < PROT_HAVE_STARTED)
            return -1;
    }
    prot_cmd_status[cmd] = PROT_HAVE_RESPONSED;
    return 0;
}