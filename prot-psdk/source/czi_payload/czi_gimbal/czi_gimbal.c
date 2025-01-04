#include "../../dji_module/widget/widget.h"
#include "dji_logger.h"
#include "../czi_payload.h"
#include "../czi_transmission/czi_transmission.h"
#include "../czi_protocol_handler/protocol_longFormatHandler.h"
#include "../czi_protocol_handler/protocol_commonHandler.h"

#include "czi_gimbal.h"
#include "../psdk_config.h"
#include "../czi_json_handler/czi_json_handler.h"

static int newTuningAngle = 0;
static int gs_GimbalTuning = 0;
// T_DjiReturnCode CziGimbal_ControlGimbalSwitch(uint8_t arg)
// {
//     printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);

//     T_ProtRspData rsp;
    
//     /* TODO packet protocol */
//     unsigned char packet[2];
//     packet[0] = GIMBAL_SWITCH;
//     packet[1] = (unsigned char)arg;
//     int len = sizeof(packet);

//     int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, (unsigned char*)packet, len, &rsp);
//     /* set queue (psdk-->prot) */
//     CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);

//     USER_LOG_INFO("control gimbal Switch is 0x%x!", GIMBAL_SWITCH);
//     return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
// }

// T_DjiReturnCode CziGimbal_ControlGimbalBright(uint8_t arg)
// {
//     printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);

//     T_ProtRspData rsp;
//     unsigned char packet[2];
//     packet[0] = GIMBAL_BRIGHT;
//     packet[1] = (unsigned char)arg;
//     int len = sizeof(packet);

//     int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, (unsigned char*)packet, len, &rsp);

//     /* set queue (psdk-->prot) */
//     CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
//     CziWidget_SetIndexValue(WIDGET_INDEX_GIMBAL_BRIGHT, packet[1]);
//     USER_LOG_INFO("control gimbal Bright is 0x%x!", GIMBAL_BRIGHT);
//     return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
// }

// T_DjiReturnCode CziGimbal_ControlGimbalAngle(int arg, int GimbalAngle)
// {
//     printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);

//     T_ProtRspData rsp;
//     gs_GimbalTuning = arg;

//     newTuningAngle = gs_GimbalTuning + GimbalAngle;
//     if(newTuningAngle>=100) {
//         newTuningAngle = 100;
//     }
//     if(newTuningAngle<=0) {
//         newTuningAngle = 0;
//     }
//     CziJsonHandler_Open(FILEPATH_JSON_STATE_PSDK, 0);
//     CziJsonHandler_WriteInt(JSON_KEY_ANGLE_TUNING, gs_GimbalTuning);
//     CziJsonHandler_Close(1);
//     /* TODO packet protocol */
//     unsigned char packet[2];
//     packet[0] = GIMBAL_ANGLE;
//     packet[1] = (unsigned char)newTuningAngle;
//     int len = sizeof(packet);

//     int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, (unsigned char*)packet, len, &rsp);

//     /* set queue (psdk-->prot) */
//     CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);

//     USER_LOG_INFO("control gimbal Angle is 0x%x!,  packet[1] is %d", GIMBAL_ANGLE, packet[1]);
//     return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

// }

T_DjiReturnCode CziGimbal_ControlGimbalAngle(int GimbalAngle)
{
    T_ProtRspData rsp;
    newTuningAngle = GimbalAngle;
    if(newTuningAngle>=100) {
        newTuningAngle = 100;
    }
    if(newTuningAngle<=0) {
        newTuningAngle = 0;
    }
    unsigned char packet[2];
    packet[0] = GIMBAL_ANGLE;
    packet[1] = (unsigned char)newTuningAngle;
    int len = sizeof(packet);
    printf("psdk send prot control gimbal turing Angle is %d!,  new angle is %d", GimbalAngle, packet[1]);
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, (unsigned char*)packet, len, &rsp);

    /* set queue (psdk-->prot) */
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);

}       

