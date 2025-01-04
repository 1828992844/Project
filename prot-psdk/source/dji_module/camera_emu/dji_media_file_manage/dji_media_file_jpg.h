/**
 ********************************************************************
 * @file    dji_media_file_jpg.h
 * @brief   This is the header file for "dji_media_file_jpg.c", defining the structure and
 * (exported) function prototypes.
 *
 * @copyright (c) 2021 DJI. All rights reserved.
 *
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 *
 *********************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PSDK_MEDIA_FILE_JPG_H
#define PSDK_MEDIA_FILE_JPG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <dji_payload_camera.h>
#include <dji_typedef.h>
#include "dji_media_file_core.h"

/* Exported constants --------------------------------------------------------*/


/* Exported types ------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------*/
bool DjiMediaFile_IsSupported_JPG(const char *filePath);

T_DjiReturnCode DjiMediaFile_GetAttrFunc_JPG(struct _DjiMediaFile *mediaFileHandle,
                                             T_DjiCameraMediaFileAttr *mediaFileAttr);

T_DjiReturnCode DjiMediaFile_GetDataOrigin_JPG(struct _DjiMediaFile *mediaFileHandle, uint32_t offset, uint16_t len,
                                               uint8_t *data, uint32_t *realLen);
T_DjiReturnCode DjiMediaFile_GetFileSizeOrigin_JPG(struct _DjiMediaFile *mediaFileHandle, uint32_t *fileSize);

T_DjiReturnCode DjiMediaFile_CreateThumbNail_JPG(struct _DjiMediaFile *mediaFileHandle);
T_DjiReturnCode DjiMediaFile_GetFileSizeThumbNail_JPG(struct _DjiMediaFile *mediaFileHandle, uint32_t *fileSize);
T_DjiReturnCode
DjiMediaFile_GetDataThumbNail_JPG(struct _DjiMediaFile *mediaFileHandle, uint32_t offset, uint16_t len,
                                  uint8_t *data, uint16_t *realLen);
T_DjiReturnCode DjiMediaFile_DestroyThumbNail_JPG(struct _DjiMediaFile *mediaFileHandle);

T_DjiReturnCode DjiMediaFile_CreateScreenNail_JPG(struct _DjiMediaFile *mediaFileHandle);
T_DjiReturnCode DjiMediaFile_GetFileSizeScreenNail_JPG(struct _DjiMediaFile *mediaFileHandle, uint32_t *fileSize);
T_DjiReturnCode
DjiMediaFile_GetDataScreenNail_JPG(struct _DjiMediaFile *mediaFileHandle, uint32_t offset, uint16_t len,
                                   uint8_t *data, uint16_t *realLen);
T_DjiReturnCode DjiMediaFile_DestroyScreenNail_JPG(struct _DjiMediaFile *mediaFileHandle);

#ifdef __cplusplus
}
#endif

#endif // PSDK_MEIDA_FILE_JPG_H

/************************ (C) COPYRIGHT DJI Innovations *******END OF FILE******/
