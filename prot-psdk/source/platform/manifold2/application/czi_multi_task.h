/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CZI_MULTI_TASK_H
#define CZI_MULTI_TASK_H

/* Includes ------------------------------------------------------------------*/
#include "dji_typedef.h"
#include "czi_multi_task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/


/* Exported types ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
T_DjiReturnCode CziMultiTask_ApplicationStart(void* args);
void CziMultiTask_NormalExitHandler(int signalNum);

#ifdef __cplusplus
}
#endif

#endif // CZI_MULTI_TASK_H