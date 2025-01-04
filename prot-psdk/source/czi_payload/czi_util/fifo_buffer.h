#ifndef _FIFO_BUFFER_H_
#define _FIFO_BUFFER_H_

typedef struct _FifoBuffer {
    int maxLen;
    int in1Len;
    char *in1;
    char *in2;
} T_FifoBuffer, *PT_FifoBuffer;

typedef int (*fifoHandler)(const char *, const int, void *arg);

void FifoBuffer_Init(PT_FifoBuffer ptFifo, const int length);
void FifoBuffer_Exit(PT_FifoBuffer ptFifo);
void FifoBuffer_Reset(PT_FifoBuffer ptFifo);
int FifoBuffer_Handle(PT_FifoBuffer ptFifo, const char *data, const int length, void *arg, fifoHandler handler);
int FifoBuffer_HandleLastData(PT_FifoBuffer ptFifo, void *arg, fifoHandler handler);

#endif