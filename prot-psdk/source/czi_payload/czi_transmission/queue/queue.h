// License-Identifier: czi-1.0
/*
* uart head file for czi MP12 device
*
* Copyright (C) 2022-2023 real-watson (291178019@qq.com)
*
*/
#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <pthread.h>
//#include "czi_uart.h"

/* macro definitions */
#define MAX_QUEUE_LEN 1024
#define QUEUE_TYPE 1
#define MAX_TEXT_LEN 1000
#define INTERNAL_SLEEP_US 20

/* @brief message queue data struct */
typedef struct {
    /**
    * mq format |mtext|len|
    *             256   4
    */
    unsigned int len;          /*data length*/

    char mtext[MAX_TEXT_LEN];  /*data buffer*/

    /*TBD*/
} T_CziQueueAssembleData;

/* @brief message queue struct */
typedef struct {
    long int mtype;                /*queue type*/
    T_CziQueueAssembleData qdata;  /*queue data*/
} T_CziQueueQueue;

/* @brief message queue setting struct */
typedef struct {
    int id;                   /*queue id*/
    int flag;                 /*queue flag*/
    int snd_mode;             /*queue send status*/
    long int mtype;
    char key_string[128];     /*key string*/
    /*TBD*/
} T_CziQueueQsetting;

typedef struct {
    T_CziQueueQsetting psdk2pickup_queue;
    T_CziQueueQsetting pickup2psdk_queue;
    T_CziQueueQsetting command_queue;
} T_CziQueueQsettingList;

/* @brief extern functions */
extern int CziQueue_InitQueue(T_CziQueueQsetting *qsetting);
extern int CziQueue_GetQueueData(T_CziQueueQueue *queue, T_CziQueueQsetting *qsetting);
extern int CziQueue_SetQueueData(const char *msg, unsigned int len, T_CziQueueQsetting *qsetting);
extern int CziQueue_ValidateQueueEmpty(struct msqid_ds *msg, T_CziQueueQsetting *qsetting);
extern int CziQueue_ResetQueue(T_CziQueueQsetting *qsetting);
#endif /*QUEUE_H_*/
