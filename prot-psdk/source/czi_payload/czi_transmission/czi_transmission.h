/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CZI_TRANSMISSION_H
#define CZI_TRANSMISSION_H

/* Includes ------------------------------------------------------------------*/
// #include "dji_typedef.h"
#include "queue/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/

typedef enum {
    QUEUE_TYPE_PILOT_TO_MSDK = 0x00,  
    QUEUE_TYPE_PSKD_TO_PROT  = 0x01,  
    QUEUE_TYPE_PROT_TO_PSKD  = 0x02,
    QUEUE_TYPE_MSDK_TO_PSKD  = 0x03
} E_QueueType;

/* Exported types ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int CziTransmission_Init(void);
int CziTransmission_GetQueueDataById(E_QueueType id, T_CziQueueQueue *ptQueue);
int CziTransmission_SetQueueDataById(E_QueueType id, const char *data, unsigned int len);
int CziTransmission_SendDataByMailbox(const char *sentData, int sentLen);
int CziTransmission_RecvDataByMailbox(char *recvData, int *recvLen);

#ifdef __cplusplus
}
#endif

#endif // CZI_TRANSMISSION_H