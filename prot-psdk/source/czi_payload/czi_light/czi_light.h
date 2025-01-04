/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CZI_LIGHT_H
#define CZI_LIGHT_H

/* Includes ------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/


/* Exported types ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

T_DjiReturnCode CziLight_ControlLight(char arg);
T_DjiReturnCode CziLight_BreathingLight(char arg);

#ifdef __cplusplus
}
#endif

#endif // CZI_LIGHT_H