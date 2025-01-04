#ifndef CZI_VIDEO_H
#define CZI_VIDEO_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "dji_liveview.h"
#include <semaphore.h>
#include "../../psdk_config.h"
#include "dji_typedef.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    E_DjiMountPosition mountPosition;
    E_DjiLiveViewCameraSource source;
} CziVideoThreadArgs;

typedef void (*DjiLiveview_H264Callback)(E_DjiLiveViewCameraPosition position, const uint8_t *buf, uint32_t len);
T_DjiReturnCode CziVideo_Init(void);
T_DjiReturnCode CziVideo_Deinit(void);
T_DjiReturnCode CziVideo_StartStream(E_DjiLiveViewCameraPosition position, E_DjiLiveViewCameraSource source);
T_DjiReturnCode CziVideo_StopStream(E_DjiLiveViewCameraPosition position, E_DjiLiveViewCameraSource source);
T_DjiReturnCode CziVideo_RequestIFrame(E_DjiLiveViewCameraPosition position, E_DjiLiveViewCameraSource source);
T_DjiReturnCode CziVideo_RunSample(int action);
void CziVideo_SetStopStreamFlag(bool value);
#ifdef __cplusplus
}
#endif

#endif // CZI_VIDEO_H
