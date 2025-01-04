/**
 ********************************************************************
 * @author yealkio
 * @file    protocol_longFormatHdl.c
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

#include "protocol_longFormatHandler.h"
#include "protocol_commonHandler.h"
// #include "czi_execute.h"


#define DEBUG_INTERNAL_LONG_FORMAT  (0)


static int Prot_ValidateData(unsigned char *uncertain, int len)
{
    int real_len;

    /* length */
    if (len < PROT_LONG_FORMAT_LENGTH_EXCEPT) {
        printf("len < PROT_LONG_FORMAT_LENGTH_EXCEPT\n");
        return -1;
    }

    /* header */
    if (PROT_HEADER_DATA != uncertain[0]) {
        printf("len < PROT_HEADER_DATA != uncertain[0]\n");
        return -1;
    }

    /* tailer */
    if (PROT_TAILER_DATA != uncertain[len - 1]) {
        printf("len < PROT_TAILER_DATA != uncertain[len - 1]\n");
        return -1;
    }

    /* validate xor */
    /**
     * @fixme quickly
     * @note The xor is temporarily 0x00.
    */
    // int xor;
    // xor = Prot_ValidateXOR(&uncertain[3], len - PROT_LONG_FORMAT_LENGTH_EXCEPT + 1);
    // if (xor != uncertain[len - 2]) {
    //     return -1;
    // }

    real_len = uncertain[1] * 256 + uncertain[2];
    if (len != real_len) {
        printf("len < len != real_len\n");
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

    rsp->protCmd = fullyData[3];

    if (len - PROT_LONG_FORMAT_LENGTH_EXCEPT > 0) {
        memcpy(rsp->rspData, &fullyData[4], len - PROT_LONG_FORMAT_LENGTH_EXCEPT);
        rsp->rspLen = len - PROT_LONG_FORMAT_LENGTH_EXCEPT;
    } else {
        rsp->rspLen = 0;
    }
    return 0;
}

typedef struct{
    uint8_t header;
    uint8_t len_high;
    uint8_t len_low;
}__attribute__((packed))T_cmdHeaderOnly;

typedef struct{
    uint8_t header;
    uint8_t len_high;
    uint8_t len_low;
    uint8_t cmd;
}__attribute__((packed))T_cmdHeaderCmd;

typedef struct{
    uint8_t checksum;
    uint8_t tail;
}__attribute__((packed))T_cmdEnd;



typedef struct{
    char     command;
    char*     body;
    int         body_len;
    int         total_len;
}T_cmdInfo;



static E_checkCMD cmdValidCheck(T_cmdHeaderCmd *head, char * buf_end, T_cmdInfo* cmd){
    T_cmdEnd *cmdEnd = NULL;
    char *body = NULL;
    char *start = (char *)head;

    int checkLen = buf_end - start;

    if(checkLen == 0){
       /* case 5: `COMMAND``C`<end>*/
        return CMD_UNKNOWN;
    }

    if(checkLen == 1){
       /* case 7: `COMMAND``CO`<end>*/
        int checkLen = (int)((head->len_high)<<8);
        if(checkLen > LEN_CMD_MAX)
            return CMD_INVALID;
        else
            return CMD_UNKNOWN;
    }
    int total_len = ((int)((head->len_high)<<8) + (int)(head->len_low));
    #ifdef DEBUG_MSG
    #if DEBUG_INTERNAL_LONG_FORMAT
    printf("total_len: %02x\r\n", total_len);
    #endif
    #endif
    if(total_len < sizeof(T_cmdHeaderCmd) + sizeof(T_cmdEnd) || total_len > LEN_CMD_MAX)
        return CMD_INVALID;
    
    if(checkLen == 2){
        /* case 6: `COMMAND``COM`<end>*/
        return CMD_UNKNOWN;
    }


    int body_len = total_len - sizeof(T_cmdHeaderCmd) - sizeof(T_cmdEnd);

    cmdEnd = (T_cmdEnd*)(start + sizeof(T_cmdHeaderCmd) + body_len);
    body = start + sizeof(T_cmdHeaderCmd);
    if(total_len > LEN_CMD_MAX){
        //printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
        return CMD_INVALID;
    }

    if(checkLen == 3){
        /*case 8: `COMMAND``COMM`<end> or  `COMMAND``COM`A<end>*/
        if(CziProt_CmdCheckValue(head->cmd) == false){
            //printf("anti failed in (%s) (%d)",__FUNCTION__, __LINE__);
            return CMD_INVALID;
        }
            
        else
            return CMD_UNKNOWN;
    }

    if((char*)cmdEnd + sizeof(T_cmdEnd) - 1 > buf_end){
        /*case 9: `COMMAND``COMMMMM`<end>*/
        return CMD_UNKNOWN;
    }

    if(cmdEnd->tail != PROT_TAILER_DATA)
        return CMD_INVALID;//incorrect
        
    if(CziProt_CmdCheckValue(head->cmd) == false){
        //printf("anti failed in (%02x)(%s) (%d)\r\n",head->cmd, __FUNCTION__, __LINE__);
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

    if((char*)cmdEnd + sizeof(T_cmdEnd) - 1 < buf_end){
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
    for(char* ptr = start_ptr; ptr <= buf_end; ptr++){
        if (*ptr == PROT_HEADER_DATA) {
            cmdHead = (T_cmdHeaderCmd*)ptr;
            #ifdef DEBUG_MSG
            #if DEBUG_INTERNAL_LONG_FORMAT
            printf("cmdHead->len_low is %02x\r\n", cmdHead->len_low);
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
            #if DEBUG_INTERNAL_LONG_FORMAT
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
static int Proto_ValidatePacket(T_ProtCommonWorkSpace *work, char *outCommand, int *commandlen){
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
        #if DEBUG_INTERNAL_LONG_FORMAT
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
                //printf("workPtr reset (%s) (%d)\r\n",__FUNCTION__, __LINE__);
                work->workLen = 0;
                work->AntiDataDoneLeft = 0;
            }else{
                /*yes, workPtr plus and return*/
                work->workPtr = scan_ptr + cmd.total_len;
                work->AntiDataDoneLeft = 0;
            }
            
            return cmd_status;
        }else if(cmd_status == CMD_UNKNOWN){
            /*TODO-copy the rest data to lastBuf, clear the workSpace(workPtr=0)*/
            work->lastBufLen = buf_end - scan_ptr + 1;
            work->AntiDataDoneLeft = 0;
            memcpy(work->lastBufData, scan_ptr, work->lastBufLen);
            #ifdef DEBUG_MSG
            printf("anti failed in (%s) (%d)\r\n",__FUNCTION__, __LINE__);
            #endif
            return cmd_status;
        }else if(cmd_status == CMD_VALID_BUT_DATA_MORE){
            /* TODO-copy the rest data to lastBuf and return means anti success */
            #ifdef DEBUG_MSG
            #if DEBUG_INTERNAL_LONG_FORMAT
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

int Prot_LongFormatAntiStickedPackets(T_ProtCommonWorkSpace *work, unsigned char *input_data, int len, unsigned char *reply, int *replyLen)
{
    int err = -1;
    Proto_addData(work, input_data, len);
    err = Proto_ValidatePacket(work, reply, replyLen);
    if(err >= CMD_VALID){
        // printf("err is %2d\r\n", err);
        return err;  
    }

    return -1;
}


int Prot_LongFormatPack(E_ProtocolCommandAll protCmd, unsigned char *input_data, short int size, T_ProtRspData *rsp)
{
    /* package data */ 
    unsigned char xor = 0;
    short int totalLen = 0;
    unsigned char xorSize = 0;

    totalLen = size + PROT_LONG_FORMAT_LENGTH_EXCEPT;
    rsp->rspData[0] = PROT_HEADER_DATA;
    rsp->rspData[1] = totalLen / 0x100;
    rsp->rspData[2] = totalLen % 0x100;
    rsp->rspData[3] = protCmd;
    
    /* size validation */
    if (size)
        memcpy(&rsp->rspData[4], input_data, size);

#if 0
    /* get xor */
    xorSize = size + 1;
    xor = Prot_ValidateXOR(&rsp->rspData[3], xorSize);
#endif

    /**
     * @warning & @note Add tail for the format based on protocol usage.
    */
    rsp->rspData[totalLen - 2] = xor;
    rsp->rspData[totalLen - 1] = PROT_TAILER_DATA;
    rsp->rspLen = totalLen;
    rsp->protCmd = protCmd;
    return totalLen;
}


int Prot_LongFormatUnpack(unsigned char *input_data, int len, T_ProtRspData *output)
{
    int err;
    T_ProtRspData input = {};
    T_ProtRspData rsp = {};
    /* Receive data */
    err = Prot_ReceiverHandler(input_data, len, &input);
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

    /* Response Data */
    err = Prot_LongFormatPack(input.protCmd, rsp.rspData, rsp.rspLen, output);
    if (-1 == err)
        return -1;
#endif
    return 0;
}
