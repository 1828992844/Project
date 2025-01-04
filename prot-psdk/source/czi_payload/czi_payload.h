/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CZI_PAYLOAD_H
#define CZI_PAYLOAD_H

/* Includes ------------------------------------------------------------------*/
#include "dji_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/
#define CZI_PAYLOAD_MODEL    "MP130V3"

typedef struct{
    int  angle;     // arm angle, unit in degree
    int  angleMax;  // arm angle limited max
    int  angleMin;  // arm angle limited min
}T_megaphone;




/* Exported types ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
T_DjiReturnCode CziPayload_ApplicationStart(void);
void CziPayload_NormalExitHandler(int signalNum);
static int Payload_VideoStream(void *arg, const int len);
int Payload_SetMediaInfo(int stats);
int Payload_ControlGimbalAngle(void *arg, const int len);
int CziPayload_SetGimbalAngle(int angle);
int mega_setAngle(int angle);
int CziPayload_BalancePower(int BatteryValue);
int CziPayload_UpdateMediaWidget(void);

#ifdef __cplusplus
}
#endif

#endif // CZI_PAYLOAD_H