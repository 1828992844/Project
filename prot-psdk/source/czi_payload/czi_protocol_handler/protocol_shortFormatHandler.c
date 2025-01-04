/**
 ********************************************************************
 * @author yealkio
 * @file    protocol_shortFormatHdl.c
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

#include "protocol_shortFormatHandler.h"
#include "protocol_commonHandler.h"
// #include "czi_execute.h"

#define DEBUG_INTERNAL_SHORT_FORMAT (0)

static int Prot_ValidateData(unsigned char *uncertain, int len)
{
    int real_len;
    // printf("(%s XOR ) len:%02x\r\n",__FUNCTION__,len);
    /* length */
    if (len < PROT_SHORT_FORMAT_LENGTH_EXCEPT) {
        return -1;
    }

    /* header */
    if (PROT_HEADER_DATA != uncertain[0]) {
        return -1;
    }

    /* short format dont has tailer */
    // if (PROT_TAILER_DATA != uncertain[len - 1]) {
    //     return -1;
    // }

    /* validate xor */
    /**
     * @fixme quickly
     * @note The xor is temporarily 0x00.
    */
    int xor;
    xor = Prot_ValidateXOR(&uncertain[2], len - PROT_SHORT_FORMAT_LENGTH_EXCEPT + 1);
    if (xor != uncertain[len - 1]) {
        #ifdef DEBUG_MSG
        #if DEBUG_INTERNAL_SHORT_FORMAT
        printf("(%s XOR fail) orig:%02x value:%02x\r\n",__FUNCTION__, uncertain[len - 1], xor);
        #endif
        #endif
        return -1;
    }

    real_len = uncertain[1];
    if (len != real_len) {
        return -1;
    }
    return 0;
}

static int Prot_ReceiverHandler(unsigned char *fullyData, unsigned short len, T_ProtRspData *rsp)
{
    int err = -1;

    /**
     * @note @warning
    */
    err = Prot_ValidateData(fullyData, len);
    if (-1 == err) {
        return -1;
    }

    rsp->protCmd = fullyData[2];

    if (len - PROT_SHORT_FORMAT_LENGTH_EXCEPT > 0) {
        #if DEBUG_INTERNAL_SHORT_FORMAT
        // printf("go there (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        #endif
        memcpy(rsp->rspData, &fullyData[3], len - PROT_SHORT_FORMAT_LENGTH_EXCEPT);
        rsp->rspLen = len - PROT_SHORT_FORMAT_LENGTH_EXCEPT;
    } else {
        #if DEBUG_INTERNAL_SHORT_FORMAT
        // printf("go there (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        #endif
        rsp->rspLen = 0;
    }
    return 0;
}



typedef struct{
    uint8_t header;
    uint8_t len;
    // uint8_t len_high;
    // uint8_t len_low;
}__attribute__((packed))T_cmdHeaderOnly;

typedef struct{
    uint8_t header;
    uint8_t len;
    // uint8_t len_high;
    // uint8_t len_low;
    uint8_t cmd;
}__attribute__((packed))T_cmdHeaderCmd;

typedef struct{
    uint8_t checksum;
    // uint8_t mark;
}__attribute__((packed))T_cmdEnd;

typedef struct{
    unsigned char     command;
    unsigned char*     body;
    int         body_len;
    int         total_len;
}T_cmdInfo;

static E_checkCMD cmdValidCheck(T_cmdHeaderCmd *head, unsigned char * buf_end, T_cmdInfo* cmd){
    T_cmdEnd *cmdEnd = NULL;
    unsigned char *body = NULL;
    unsigned char *start = (unsigned char *)head;

    int checkLen = buf_end - start;
    #if DEBUG_INTERNAL_SHORT_FORMAT
    printf("checkLen in (%s) is (%d)\r\n",__FUNCTION__, checkLen);
    #endif
    if(checkLen == 0){
       /* case 5: `COMMAND``C`<end>*/
        #if DEBUG_INTERNAL_SHORT_FORMAT
        printf("go there (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        #endif
        return CMD_UNKNOWN;
    }

    if(checkLen == 1){
       /* case 7: `COMMAND``CO`<end>*/
        int checkLen = (unsigned char)((head->len));
        if(checkLen > LEN_CMD_MAX)
            return CMD_INVALID;
        else{
            #if DEBUG_INTERNAL_SHORT_FORMAT
            printf("go there (%s) (%d)\r\n",__FUNCTION__, __LINE__);
            #endif
            return CMD_UNKNOWN;
        }

    }
    
    int total_len = ((unsigned char)(head->len));
    #if DEBUG_INTERNAL_SHORT_FORMAT
    printf("total_len in (%s) is (%d)\r\n",__FUNCTION__, total_len);
    #endif
    if(total_len < sizeof(T_cmdHeaderCmd) + sizeof(T_cmdEnd) || total_len > LEN_CMD_MAX)
        return CMD_INVALID;
    
    if(checkLen == 2){
        /* case 6: `COMMAND``COM`<end>*/
        #if DEBUG_INTERNAL_SHORT_FORMAT
        printf("go there (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        #endif
        return CMD_UNKNOWN;
    }


    int body_len = total_len - sizeof(T_cmdHeaderCmd) - sizeof(T_cmdEnd);
    #if DEBUG_INTERNAL_SHORT_FORMAT
    printf("body_len in (%s) is (%d)\r\n",__FUNCTION__, body_len);
    #endif
    cmdEnd = (T_cmdEnd*)(start + sizeof(T_cmdHeaderCmd) + body_len);
    body = start + sizeof(T_cmdHeaderCmd);
    if(total_len > LEN_CMD_MAX){
        #if DEBUG_INTERNAL_SHORT_FORMAT
        printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        #endif
        return CMD_INVALID;
    }

    if(checkLen == 3){
        /*case 8: `COMMAND``COMM`<end> or  `COMMAND``COM`A<end>*/
        if(CziProt_CmdCheckValue(head->cmd) == false){
            #if DEBUG_INTERNAL_SHORT_FORMAT
            printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
            #endif
            return CMD_INVALID;
        }  
        else{  //this issue is correct in short format
            cmd->command = head->cmd;
            cmd->body = body;
            cmd->body_len = body_len;
            cmd->total_len = total_len;
            return CMD_VALID;//correct
        }
            
    }

    if((unsigned char*)cmdEnd + sizeof(T_cmdEnd) - 1 > buf_end){
        /*case 9: `COMMAND``COMMMMM`<end>*/
        #if DEBUG_INTERNAL_SHORT_FORMAT
        printf("go there (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        #endif
        return CMD_UNKNOWN;
    }
    // if(cmdEnd->mark != PROT_TAILER_DATA)
    //     return CMD_INVALID;//incorrect

        
    if(CziProt_CmdCheckValue(head->cmd) == false){
        #if DEBUG_INTERNAL_SHORT_FORMAT
        printf("anti failed in (%02x)(%s) (%d)\r\n",head->cmd, __FUNCTION__, __LINE__);
        #endif
        return CMD_INVALID;
    }
        
#if 0   
    if (cmdCheckLen(head->cmd, body, total_len))
        return CMD_INVALID;
#endif
    //set cmd struct
    cmd->command = head->cmd;
    cmd->body = body;
    cmd->body_len = body_len;
    cmd->total_len = total_len;

    if((char*)cmdEnd + sizeof(T_cmdEnd) - 1 < (char*)buf_end){
        /*case 10: ABC`COMMAND``COMMAND`D*/
        return CMD_VALID_BUT_DATA_MORE;
    }
    return CMD_VALID;//correct
}


/*
case 1: ABC`COMMAND`D
case 2: ABCD
case 3: ADC`COMMA`<end>
case 4: <start>`ND`ABC
case 5: `COMMAND``C`<end>
case 6: `COMMAND``COM`<end>
case 7: `COMMAND``CO`<end>
case 8: `COMMAND``COMM`<end>
case 9: `COMMAND``COMMMMM`<end>
case 10: ABC`COMMAND``COMMAND`D
case 11: `COMMAN`<end> + <start>`D`
return offset according to start_ptr;
*/
static unsigned int cmdValidSearch(char *start_ptr, char *buf_end, T_cmdInfo* cmd, E_checkCMD *status){
    if(start_ptr > buf_end){
        return buf_end - start_ptr + 1; //go to the end + 1
    }
    
    //1. scan the header
    T_cmdHeaderCmd *cmdHead = NULL;
    for(unsigned char* ptr = start_ptr; ptr <= (unsigned char*)buf_end; ptr++){
        if (*ptr == PROT_HEADER_DATA) {
            cmdHead = (T_cmdHeaderCmd*)ptr;
            #ifdef DEBUG_MSG
            #if DEBUG_INTERNAL_SHORT_FORMAT
            printf("cmdHead->len is %02x\r\n", cmdHead->len);
            printf("cmdHead->header is %02x\r\n", cmdHead->header);
            printf("cmdHead->cmd is %02x\r\n", cmdHead->cmd);
            #endif
            #endif
            break;
        }
    }
    if(cmdHead != NULL){
        *status = cmdValidCheck(cmdHead, buf_end, cmd);
        if(*status == CMD_VALID){//command valid
            return (char*)cmdHead - start_ptr;
        }
        else if(*status == CMD_VALID_BUT_DATA_MORE){
            #ifdef DEBUG_MSG
            #if DEBUG_INTERNAL_SHORT_FORMAT
            printf("%s call CMD_VALID_BUT_DATA_MORE in line %d\r\n",__FUNCTION__, __LINE__);
            #endif
            #endif
            return (char*)cmdHead - start_ptr;
        }
        else if(*status == CMD_UNKNOWN){
            return (char*)cmdHead - start_ptr;
        }
        else{//command invalid
            printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
            return 1;
        }
    }else{
        return buf_end - start_ptr + 1; //go to the end + 1
    }
}


/**
 * @brief validate packet
 * @param need_valid, data to be valided
 * @param len, length of validated packet
 * @return -1 if no valid command; if has next packet to do return 0;
 */
static int Proto_ValidatePacket(T_ProtCommonWorkSpace *work, unsigned char *outCommand, int *commandlen){
    if(work->workLen == 0){
        //printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        return -1;
    }

    unsigned char *scan_ptr = work->workPtr;
    unsigned char *buf_end = &work->workspace[work->workLen - 1];
    int restWorkLen = buf_end - scan_ptr + 1;
    
    if(restWorkLen <= 0){
        //printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        return -1;
    }
        
    
    T_cmdInfo cmd = {};
    E_checkCMD cmd_status;
    while(scan_ptr <= buf_end){
        unsigned int offset = cmdValidSearch(scan_ptr, buf_end, &cmd, &cmd_status);
        #ifdef DEBUG_MSG
        #if DEBUG_INTERNAL_SHORT_FORMAT
        printf("offset is %02d cmd.total_len: %d\r\n", offset, cmd.total_len);
        #endif
        #endif
        scan_ptr += offset;
        if(scan_ptr > buf_end)
            break;
        if(cmd_status == CMD_VALID){
            memcpy(outCommand, scan_ptr, cmd.total_len);
            *commandlen = cmd.total_len;
            /*rest has many data?*/
            if(scan_ptr + cmd.total_len > buf_end){
                /*no, workPtr set 0 and return*/
                // printf("workPtr reset (%s) (%d)\r\n",__FUNCTION__, __LINE__);
                work->workLen = 0;
                work->AntiDataDoneLeft = 0;
            }else{
                /*yes, workPtr plus and return*/
                work->workPtr = scan_ptr + cmd.total_len;
                work->AntiDataDoneLeft = 0;
            }
            
            return CMD_VALID;
        }else if(cmd_status == CMD_UNKNOWN){
            /*TODO-copy the rest data to lastBuf, clear the workSpace(workPtr=0)*/
            work->lastBufLen = buf_end - scan_ptr + 1;
            work->AntiDataDoneLeft = 0;
            memcpy(work->lastBufData, scan_ptr, work->lastBufLen);
            #ifdef DEBUG_MSG
            printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
            #endif
            return CMD_UNKNOWN;
        }else if(cmd_status == CMD_VALID_BUT_DATA_MORE){
            /* TODO-copy the rest data to lastBuf and return means anti success */
            #ifdef DEBUG_MSG
            #if DEBUG_INTERNAL_SHORT_FORMAT
            printf("(%s) anti success but data is more in line (%d) (%d)\r\n",__FUNCTION__, __LINE__, cmd_status);
            #endif
            #endif
            memcpy(outCommand, scan_ptr, cmd.total_len);
            *commandlen = cmd.total_len;
            work->workPtr = scan_ptr + cmd.total_len;
            work->AntiDataDoneLeft = buf_end - work->workPtr + 1;
            memcpy(work->lastBufData, scan_ptr + cmd.total_len, work->AntiDataDoneLeft);
            return CMD_VALID_BUT_DATA_MORE;
        }else{
            /*invalid, continue to find valid*/
            //printf("anti failed in (%d) (%s) (%d)\r\n",cmd_status, __FUNCTION__, __LINE__);
            work->AntiDataDoneLeft = 0;
        }
    }

    work->workLen = 0;
    //printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
    return -1;
}

static int Proto_addData(T_ProtCommonWorkSpace *work, unsigned char *inData, int dataLen){
   
     /*1. check old buffer has data or not?*/
    memset(work->workspace, 0, sizeof(work->workspace));

    if(work->AntiDataDoneLeft > 0){
        memcpy(work->workspace, work->lastBufData, work->AntiDataDoneLeft);
        work->workLen = work->AntiDataDoneLeft;
        work->workPtr = work->workspace;
        return 0;
    }

    if(work->lastBufLen > 0){
        /*yes, re-packet data, check len and send to decode*/
        memcpy(work->workspace, work->lastBufData, work->lastBufLen);
        memcpy(work->workspace + work->lastBufLen, inData, dataLen);
        work->workLen = work->lastBufLen + dataLen;
        work->lastBufLen = 0;
    }else{
        /*no, clear the workspace, and add the new data in it*/
        memcpy(work->workspace, inData, dataLen);
        work->workLen = dataLen;
    }
    work->workPtr = work->workspace;
    //printf("work->workLen = %d\r\n", work->workLen);
    return 0;
}

int Prot_ShortFormatAntiStickedPackets(T_ProtCommonWorkSpace *work, unsigned char *data, int len, unsigned char *reply, int *replyLen)
{
    int err = -1;
    Proto_addData(work, data, len);
    err = Proto_ValidatePacket(work, reply, replyLen);
    if(err >= CMD_VALID){
        // printf("err is %2d\r\n", err);
        return err;  
    }

    return -1;
}


int Prot_ShortFormatPack(E_ProtocolCommandAll protCmd, unsigned char *data, short int size, T_ProtRspData *rsp)
{
    /* package data */ 
    unsigned char xor = 0;
    short int total_len = 0;
    unsigned char xor_size = 0;

    /**
     * @note Based on receiver's handler,
     * the plus-1 is clear for size,
     * we need that length of command which is one byte for the reason that
     * the size is just the length of naked data who gives.
     */
    total_len = size + PROT_SHORT_FORMAT_LENGTH_EXCEPT;

    rsp->rspData[0] = PROT_HEADER_DATA;
    rsp->rspData[1] = total_len;
    rsp->rspData[2] = protCmd;
    
    /* size validation */
    if ( size )
        memcpy(&rsp->rspData[3],data,size);

    /* get xor */
    xor_size = size + 1; // The 1 is cmd
    xor = Prot_ValidateXOR(&rsp->rspData[2],xor_size);
    rsp->rspData[total_len - 1] = xor; //means last data

    /**
     * @warning & @note Take a big warning look at here.
     * Based on DJi's transmission link, the end of data should be appended
     * with null, so the length of data is bigger one byte than the before.
     * Avoid too much bugs caused by this 'dirty' length of transmission link,
     * be silent and add 1 byte to the total length.
    */
    rsp->rspLen = total_len;

    rsp->protCmd = protCmd;

    return total_len;
}


int Prot_ShortFormatUnpack(unsigned char *data, int len, T_ProtRspData *output)
{
    int err;
    T_ProtRspData input = {};
    T_ProtRspData rsp = {};
    /* Receive data */
    err = Prot_ReceiverHandler(data, len, &input);
#if 1    
    if(err){
        printf("Prot_ReceiverHandler err %2d\r\n", err);
        return -1;
    }
    else
        memcpy(output, &input, sizeof(T_ProtRspData));
#else
    /* running cmd hdl */
    err = CziExe_Run(input.protCmd, input.rspData, input.rspLen, &rsp);
    if(err == -1)
        return -1;

    if(rsp.synced == 1){
        output->synced = 1;
        return 0;
    }

    /* Response Data */
    err = Prot_ShortFormatPack(input.protCmd, rsp.rspData, rsp.rspLen, output);
    if (-1 == err)
        return -1;
#endif
    return 0;
}
