// License-Identifier: czi-1.0
/*
* uart head file for czi MP12 device
*
* Copyright (C) 2022-2023 real-watson (291178019@qq.com)
*
*/
#include "queue.h"

/**
* @brief validate queue
* @param msg T_CziQueueQsetting
* @return if = 0, not failed; if < 0, failed
*/
int CziQueue_ValidateQueueEmpty(struct msqid_ds *msg, T_CziQueueQsetting *qsetting) {
    /*
    * set IPC_STAT
    * always return
    */
    if(msgctl(qsetting->id, IPC_STAT, msg)) {
        perror("msgctl");
        return -1;
    }

    return 0;
}

/**
* @brief validate queue
* @param msg T_CziQueueQsetting
* @return if = 0, not failed; if < 0, failed
*/
int CziQueue_ResetQueue(T_CziQueueQsetting *qsetting) {
    /*
    * set IPC_STAT
    * always return
    */
    struct msqid_ds msg;
    if(msgctl(qsetting->id, IPC_RMID, &msg)) {
        perror("msgctl");
        return -1;
    }

    return 0;
}

/**
* @brief Set data to queue
* @param msg, data from users
* @param len, data length
* @param qsetting, T_CziQueueQsetting data
* @return if = 0, not failed; if < 0, failed
*/
int CziQueue_SetQueueData(const char *msg, unsigned int len, T_CziQueueQsetting *qsetting) {
    T_CziQueueQueue queue;
    memset(&queue,0,sizeof(T_CziQueueQueue));

    /* validate len */
    if ( !len || len >= MAX_TEXT_LEN) {
        perror("MAX_TEXT_LEN");
        return -1;
    }

//set mtype `must be`
    queue.mtype     = qsetting->mtype;
    queue.qdata.len = len;
    memcpy(queue.qdata.mtext,msg,len);

    /* set IPC_NOWAIT  always return */

    if(msgsnd(qsetting->id,(void *)&queue,sizeof(queue.qdata),qsetting->snd_mode) < 0) {
        perror("msgsnd");
        return -1;
    }

    return 0;
}

/**
* @brief Get data from queue
* @param queue, struct of T_CziQueueQueue
* @param qsetting, struct of T_CziQueueQsetting
* @return if = 0, not failed; if < 0, failed
*/
int CziQueue_GetQueueData(T_CziQueueQueue *queue,T_CziQueueQsetting *qsetting) {
    int ret = -1;


    ret = msgrcv(qsetting->id,(void *)queue,MAX_QUEUE_LEN,qsetting->mtype,0);
    if ( -1 == ret ) {
//perror("receive msg error");
        return -1;
    }

    /*
    * check return data
    */
    if ( !queue->qdata.len ) {
        perror("qdata len error");
        return -1;
    }

    return 0;
}

/**
* @brief Init queue
* @param qsetting, T_CziQueueQsetting data
* @return if id > 0, not failed; if < 0, failed
*/
int CziQueue_InitQueue(T_CziQueueQsetting *qsetting) {
    int id = -1;
    int ret = -1;

    key_t key = ftok(qsetting->key_string, qsetting->flag);

//get key
    id = msgget(key,IPC_CREAT | 0666);
    if(id == -1) {
        perror("msgget");
        return -1;
    }
    printf("==============the real is %d\n",id);

    /*set qsetting*/
    qsetting->id = id;


    return 0;
}

int test_main(void) {
    int id;                   /*queue id*/
    int flag;                 /*queue flag*/
    int snd_mode;             /*queue send status*/
    char key_string[128];     /*key string*/

    int ret= -1;


    T_CziQueueQsetting recv_data_queue = {
        .id = 0,
        .flag = 16,
        .snd_mode = IPC_NOWAIT,
        .mtype = 1,
        .key_string = "/tmp",
    };

    T_CziQueueQsetting send_data_queue = {
        .id = 0,
        .flag = 16,
        .snd_mode = IPC_NOWAIT,
        .mtype = 2,
        .key_string = "/tmp",
    };


    ret = CziQueue_InitQueue(&recv_data_queue);
    ret = CziQueue_InitQueue(&send_data_queue);

    while(1) {
        T_CziQueueQueue queue =  { 0 };
        char *data = "hello psdk , I am pickup";
        ret = CziQueue_SetQueueData(data,strlen(data),&send_data_queue);

        ret = CziQueue_GetQueueData(&queue,&recv_data_queue);
        if ( queue.qdata.len ) {
            printf("queue.qdata.mtext:%s\n",queue.qdata.mtext);
        }

        sleep(1);
    }
}
