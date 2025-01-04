/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CZI_MSDK_HANDLER_H
#define CZI_MSDK_HANDLER_H

/* Includes ------------------------------------------------------------------*/
#include "dji_typedef.h"

// #include "msdk_protobuf.pb-c.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/
typedef enum _MediaChannelType {
    MEDIA_CHANNEL_TYPE_MASTER     = 0x00,
    MEDIA_CHANNEL_TYPE_BACKGROUND = 0x01
} E_MediaChannelType;

/* Exported types ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
T_DjiReturnCode CziMsdkHandler_Init(void);
T_DjiReturnCode CziMegaphone_UpdateMusicList(const char *filename);

#ifdef __cplusplus
}
#endif

#endif // CZI_MSDK_HANDLER_H