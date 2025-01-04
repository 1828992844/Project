#include "queue/queue.h"
#include "mailbox/czi_mailbox.h"
#include "czi_transmission.h"

#define MAX_QUEUE_NUM    10

static T_CziQueueQsetting gs_arrTCziQueueQsetting[MAX_QUEUE_NUM] = {0x00};

static int Transmission_InitQueue(void)
{
    memset(gs_arrTCziQueueQsetting, 0x00, sizeof(gs_arrTCziQueueQsetting));

    T_CziQueueQsetting pilot2Msdk = {
        .id = 0,
        .flag = 16,
        .snd_mode = IPC_NOWAIT,
        .mtype = 1,
        .key_string = "psdk",
    };
    CziQueue_InitQueue(&pilot2Msdk);
    memcpy(&gs_arrTCziQueueQsetting[QUEUE_TYPE_PILOT_TO_MSDK], &pilot2Msdk, sizeof(T_CziQueueQsetting));

    T_CziQueueQsetting psdk2Prot = {
        .id = 0,
        .flag = 1,
        .snd_mode = 0,
        .mtype = 2,
        .key_string = "psdk",
    };
    CziQueue_InitQueue(&psdk2Prot);
    memcpy(&gs_arrTCziQueueQsetting[QUEUE_TYPE_PSKD_TO_PROT],  &psdk2Prot, sizeof(T_CziQueueQsetting));

    T_CziQueueQsetting prot2Psdk = {
        .id = 0,
        .flag = 1,
        .snd_mode = 0,
        .mtype = 3,
        .key_string = "psdk",
    };
    CziQueue_InitQueue(&prot2Psdk);
    memcpy(&gs_arrTCziQueueQsetting[QUEUE_TYPE_PROT_TO_PSKD],  &prot2Psdk, sizeof(T_CziQueueQsetting));

    T_CziQueueQsetting msdk2Psdk = {
        .id = 0,
        .flag = 1,
        .snd_mode = 0,
        .mtype = 4,
        .key_string = "psdk",
    };
    CziQueue_InitQueue(&msdk2Psdk);
    memcpy(&gs_arrTCziQueueQsetting[QUEUE_TYPE_MSDK_TO_PSKD],  &msdk2Psdk, sizeof(T_CziQueueQsetting));

    return 0;
}

int CziTransmission_Init(void)
{
    Transmission_InitQueue();

    T_CziMailboxComms original   = {0x00};
    T_CziMailboxComms targetProt = {0x00};

    CziMailbox_InitOriginal(&original, MAILBOX_COMMS_HANDLER_PSDK, MAILBOX_COMMS_PSDK);
    CziMailbox_InitTarget(&targetProt, MAILBOX_COMMS_HANDLER_PROT, MAILBOX_COMMS_PROT);

    return 0;
}

int CziTransmission_GetQueueDataById(E_QueueType id, T_CziQueueQueue *ptQueue)
{
    T_CziQueueQsetting tQsetting = {0x00};
    memcpy(&tQsetting, &gs_arrTCziQueueQsetting[id], sizeof(T_CziQueueQsetting));

    memset(ptQueue, 0x00, sizeof(T_CziQueueQueue));
    CziQueue_GetQueueData(ptQueue, &tQsetting);

    return 0;
}

int CziTransmission_SetQueueDataById(E_QueueType id, const char *data, unsigned int len)
{
    CziQueue_SetQueueData(data, len , &gs_arrTCziQueueQsetting[id]);

    return 0;
}

int CziTransmission_SendDataByMailbox(const char *sentData, int sentLen)
{
   int ret = CziMailbox_SendData(MAILBOX_COMMS_HANDLER_PROT, (char *)sentData, sentLen);

   return 0;
}

int CziTransmission_RecvDataByMailbox(char *recvData, int *recvLen)
{
    int errCode = CziMailbox_RecvData(MAILBOX_COMMS_HANDLER_PSDK, recvData, recvLen);
    if (-1 == errCode) {
    }

    return 0;
}