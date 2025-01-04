/**
 ********************************************************************
 * @author yealkio
 * @file    czi_protocol.c
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
#include "czi_protocol.h"

#include "protocol_channelRegister.h"
#include "protocol_commonHandler.h"
#include "protocol_longFormatHandler.h"
#include "protocol_shortFormatHandler.h"

int CziProt_ConfigurationInit(T_CziProtConfiguration *protCfg,T_CziProtHandler *protHdl)
{
    T_ProtChannelConfig channel = {};
    T_ProtChannelHdl *chnHdl = (T_ProtChannelHdl *)malloc(sizeof(T_ProtChannelHdl)); //Request memory for channel.

    if (NULL == chnHdl)
        return -1;

    channel.channelNum = protCfg->protDataChannel;
    channel.type       = protCfg->type;
    protHdl->type = channel.type;
    Prot_ChannelRegister(&channel, chnHdl);  //initialize channel.

    protHdl->channelHdl = chnHdl;
    protHdl->sendCallback = protCfg->sendCallback; //set sender callback function.
    return 0;
}

int CziProt_HandlerDeInit(T_CziProtHandler *protHdl)
{
    if (protHdl != NULL) {
        if (protHdl->channelHdl->workSpace != NULL) {
            free(protHdl->channelHdl->workSpace); //free protHdl workSpace.
            protHdl->channelHdl->workSpace = NULL; 
        }

        if (protHdl->channelHdl != NULL) {
            free(protHdl->channelHdl); //free protHdl channel.
            protHdl->channelHdl = NULL; 
        }

        free(protHdl); //free protHdl.
    }
    return 0;
}

int CziProt_DataUnpackHdl(T_CziProtHandler *protHdl, unsigned char *raw_data, int len)
{
    /* initialize tmp variable*/
    int errCode = -1;
    unsigned char reply[PROT_MAX_LEN] = {};
    int replyLen;
    T_ProtRspData hdlRsp = {};
    T_ProtChannelWorkSpace *work = NULL;

    /* check legal input param */
    if(protHdl == NULL)
        printf("(%s) protHdl is NULL (LINE:%d)\n", __FUNCTION__, __LINE__);

    work = protHdl->channelHdl->workSpace;

    if(work == NULL)
        printf("(%s) work is NULL (LINE:%d)\n", __FUNCTION__, __LINE__);

    /* anti packet handler */
#if 1

    switch (protHdl->type) {
        case PROTOCOL_SHORT_FORMAT:
        {
            /* anti packet handler */
            errCode = Prot_ShortFormatAntiStickedPackets(work, raw_data, len, reply, &replyLen);
            #ifdef DEBUG_MSG
            #if 0
            printf("(%s) len is %2d  replyLen is %2d  errcode is %d . line:(%d) \r\n", __FUNCTION__, len, replyLen, errCode, __LINE__);
            #endif
            #endif
            if (errCode < CMD_VALID) {
                printf("anti failed in channel %d\r\n", protHdl->channelHdl->protDataChannal);
                return -1;
            }
            else if( errCode ==  CMD_VALID_BUT_DATA_MORE){
                #ifdef DEBUG_MSG
                int offset = work->AntiDataDoneLeft;
                printf("anti success but do it again with %d rest data: ", offset);
                for(int i=0; i<offset; i++){
                    printf("%02x ", work->lastBufData[i]);
                }
                printf("\r\n");
                #endif
            }
            /* Data format check */
            if(Prot_ShortFormatUnpack(reply, replyLen, &hdlRsp))
                return -1;
            break;
        }

        case PROTOCOL_LONG_FORMAT:
        {
            /* anti packet handler */
            errCode = Prot_LongFormatAntiStickedPackets(work, raw_data, len, reply, &replyLen);
            #ifdef DEBUG_MSG
            #if 0
            printf("(%s) len is %2d  replyLen is %2d  errcode is %d . line:(%d) \r\n", __FUNCTION__, len, replyLen, errCode, __LINE__);
            #endif
            #endif
            if (errCode < CMD_VALID) {
                printf("anti failed in channel %d\r\n", protHdl->channelHdl->protDataChannal);
                return -1;
            }
            else if( errCode ==  CMD_VALID_BUT_DATA_MORE){
                #ifdef DEBUG_MSG
                int offset = work->AntiDataDoneLeft;
                printf("anti success but do it again with %d rest data: ", offset);
                for(int i=0; i<offset; i++){
                    printf("%02x ", work->lastBufData[i]);
                }
                printf("\r\n");
                #endif
            }
            /* Data format check */
            if(Prot_LongFormatUnpack(reply, replyLen, &hdlRsp))
                return -1;
            break;
        }
        default:
            printf("wrong protocol type %d  line:%d\r\n", protHdl->type, __LINE__);
            break;
    }

#endif

    /* Data handler */
    // NO, just unpack data and return errcode in this function

    memcpy(&protHdl->rsp, &hdlRsp, sizeof(T_ProtRspData));
    #if 0
    printf("protHdl->rsp cmd:%02x with %d rest data: ", hdlRsp.protCmd, hdlRsp.rspLen);
    for(int i=0; i<hdlRsp.rspLen; i++){
        printf("%02x ", hdlRsp.rspData[i]);
    }
    printf("\r\n");
    #endif
    return errCode;
}



int CziProt_UserDataHdl(T_CziProtHandler *protHdl, unsigned char *raw_data, int len)
{
    /* initialize tmp variable*/
    int errCode = -1;
    unsigned char reply[PROT_MAX_LEN] = {};
    T_ProtRspData hdlRsp = {};
    T_ProtRspData rspNeedToSend = {};
    T_ProtRspData *hdlRsp_tmp;

    /* check legal input param */
    if(protHdl == NULL){
        printf("(%s) protHdl is NULL (LINE:%d)\n", __FUNCTION__, __LINE__);
        return -1;
    }
        
    // T_ProtChannelWorkSpace *work = protHdl->channelHdl->workSpace;
    if(len <= 0){
        printf("(%s) invalid data len (LINE:%d)\n", __FUNCTION__, __LINE__);
        return -1;
    }

    int ret_unpacket = -1;
    do{  //loop this data frame.
        ret_unpacket = -1;

        /* Unpack Raw Data handler */
        ret_unpacket = CziProt_DataUnpackHdl(protHdl, raw_data, len);
        if(ret_unpacket < CMD_VALID)
            return -1;

        #if ENABLE_CHECK_CMD_STATUS
        err = Prot_CheckRspStatus(input.protCmd,input.rspData[0]);
        if(err){
            printf("%02x  %02x Prot_CheckRspStatus fail %d\r\n",input.protCmd,input.rspData[0], __LINE__);
            return -1;
        }
        #endif

        hdlRsp_tmp = &protHdl->rsp;
        /* running cmd hdl */
        // errCode = CziExe_Run(hdlRsp_tmp->protCmd, hdlRsp_tmp->rspData, hdlRsp_tmp->rspLen, &hdlRsp);
        // if(errCode == -1)
        //     return -1;

        /* Response Data */
        errCode = Prot_CommonPacketHandler(protHdl->type, hdlRsp_tmp->protCmd, hdlRsp.rspData, hdlRsp.rspLen, &rspNeedToSend);
        if (-1 == errCode)
            return -1;

        #if 1
        /* Send data to specific channel or device */
        if(hdlRsp.rspLen > 0)//must check reply len
            protHdl->sendCallback(rspNeedToSend.rspData, rspNeedToSend.rspLen);
        else
            printf("hdlRsp.rspLen:%d is wrong \n", hdlRsp.rspLen);
        #else
        memcpy(protHdl->userData, &hdlRsp, sizeof(T_ProtRspData));
        #endif
    }
    while(ret_unpacket > CMD_VALID);


    return 0;
}
