#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "czi_log.h"
#include "fifo_buffer.h"

#define LOG_TAG_FIFO_BUFFER    "FIFO_BUFFER"

void FifoBuffer_Init(PT_FifoBuffer ptFifo, const int length)
{
    if (ptFifo->in1) {
        free(ptFifo->in1);
        ptFifo->in1 = NULL;
    }
    ptFifo->in1 = calloc(length, sizeof(char));

    if (ptFifo->in2) {
        free(ptFifo->in2);
        ptFifo->in2 = NULL;
    }
    ptFifo->in2 = calloc(length, sizeof(char));

    ptFifo->in1Len = 0;
    ptFifo->maxLen = length;
}

void FifoBuffer_Exit(PT_FifoBuffer ptFifo)
{
    if (ptFifo->in1) {
        free(ptFifo->in1);
        ptFifo->in1 = NULL;
    }
    ptFifo->in1Len = 0;

    if (ptFifo->in2) {
        free(ptFifo->in2);
        ptFifo->in2 = NULL;
    }
}

void FifoBuffer_Reset(PT_FifoBuffer ptFifo)
{
    int maxLen = ptFifo->maxLen;
    memset(ptFifo->in1, 0, maxLen);
    memset(ptFifo->in2, 0, maxLen);
    ptFifo->in1Len = 0;
}

int FifoBuffer_Handle(PT_FifoBuffer ptFifo, const char *data, const int length, void *arg, fifoHandler handler)
{
    int maxLen = ptFifo->maxLen;
    int len = length;
    if (len + ptFifo->in1Len >= maxLen) {
        int cp_len = maxLen - ptFifo->in1Len;

        memcpy(ptFifo->in1 + ptFifo->in1Len, data, cp_len);
        //copy fifo1 to fifo2
        memset(ptFifo->in2, 0, maxLen);
        memcpy(ptFifo->in2, ptFifo->in1, maxLen);

        if (handler(ptFifo->in2, maxLen, arg)) {
            // CziLog_Error("(%s LINE-%d) run handler failed\n", __FUNCTION__, __LINE__);
            CziLog_Error( "run handler failed\n");
            return -1;
        }
        ptFifo->in1Len = 0;
        int restLen = len - cp_len;
        while (restLen >= maxLen) {
            memset(ptFifo->in2, 0, maxLen);
            memcpy(ptFifo->in2, data + cp_len, maxLen);
            if (handler(ptFifo->in2, maxLen, arg)) {
                // CziLog_Error("(%s LINE-%d) run handler failed\n", __FUNCTION__, __LINE__);
                CziLog_Error( "run handler failed\n");
                return -1;
            }
            cp_len += maxLen;
            restLen = len - cp_len;
        }

        if (restLen > 0) {
            memset(ptFifo->in1, 0, maxLen);
            memcpy(ptFifo->in1, data + cp_len, restLen);
            ptFifo->in1Len = restLen;
        }
        
    }
    else {
        //direct copy to fifo1
        memcpy(ptFifo->in1 + ptFifo->in1Len, data, len);
        ptFifo->in1Len += len;
    }

    return 0;
#if 0
    int maxLen = ptFifo->maxLen;
    if(length + ptFifo->in1Len >= maxLen)
    {
        int cp_len = maxLen - ptFifo->in1Len;

        memcpy(ptFifo->in1 + ptFifo->in1Len, data, cp_len);
        //copy fifo1 to fifo2
        memset(ptFifo->in2, 0, maxLen);
        memcpy(ptFifo->in2, ptFifo->in1, maxLen);

        handle(ptFifo->in2, maxLen);

        if(length + ptFifo->in1Len > maxLen){
            //copy the rest in fifo1 at the beginning
            int restLen = length - cp_len;
            if(restLen >= maxLen){
                ptFifo->in1Len = 0;
                handle(ptFifo->in2, maxLen);
            }
            else{
                memcpy(ptFifo->in1, data + cp_len, restLen);
                ptFifo->in1Len = restLen;
            }
        }
        else {
            ptFifo->in1Len = 0;
            memset(ptFifo->in1, 0, maxLen);
        }
    }
    else{
        //direct copy to fifo1
        memcpy(ptFifo->in1 + ptFifo->in1Len, data, length);
        ptFifo->in1Len += length;
    }
#endif
}

int FifoBuffer_HandleLastData(PT_FifoBuffer ptFifo, void *arg, fifoHandler handler)
{
//    printf("ptFifo->in1Len: %d\n", ptFifo->in1Len);
    if (ptFifo->in1Len) {
//        handler(ptFifo->in1, ptFifo->in1Len);
        if (handler(ptFifo->in1, ptFifo->in1Len, arg)) {
            // CziLog_Error("(%s LINE-%d) run handler failed\n", __FUNCTION__, __LINE__);
            CziLog_Error( "run handler failed\n");
            return -1;
        }
    }
    return 0;
}