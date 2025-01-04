/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CZI_PSDK_HANDLER_H
#define CZI_PSDK_HANDLER_H

/* Includes ------------------------------------------------------------------*/
#include "dji_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/


/* Exported types ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
T_DjiReturnCode CziPsdkHandler_Init(void);
void CziPsdkHandler_NormalExitHandler(void);
// int CziPsdkHandler_Video();

#ifdef __cplusplus 
}
#endif

#endif // CZI_PSDK_HANDLER_H