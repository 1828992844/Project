#include "../../dji_module/widget/widget.h"
#include "dji_logger.h"
#include "../czi_payload.h"
#include "../czi_transmission/czi_transmission.h"
#include "../czi_protocol_handler/protocol_longFormatHandler.h"
#include "../czi_protocol_handler/protocol_commonHandler.h"

#include "czi_light.h"

// T_DjiReturnCode CziLight_ControlLight(char arg)
// {
//     USER_LOG_INFO("Set light type is packet[1] is %d\n", arg);
//     USER_LOG_INFO("Set light type is packet[1] is %x\n", arg);
//     USER_LOG_INFO("Set light type is packet[1] is %s\n", arg);
//     printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
//     T_ProtRspData rsp;
//     unsigned char Val[2];
//     int len = 0;
//     memset(Val, 0x00, 2);
//     if(arg == 0x00) {
//         Val[0] = 0xA1;
//         len = 1;
//     }else {
//         Val[0] = 0xA0,
//         Val[1] = arg - 1;
//         len = sizeof(Val);
//     }


//     /* TODO packet protocol */
    
//     int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, Val, len, &rsp);
    
//     /* set queue (psdk-->prot) */
//     CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);

//     USER_LOG_INFO("Set light type is ok!");
//     USER_LOG_INFO("Set light type is packet[1] is %d\n", Val);
//     USER_LOG_INFO("Set light type is packet[1] is %x\n", Val);
//     USER_LOG_INFO("Set light type is packet[1] is %s\n", Val);
//     return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
// }


// T_DjiReturnCode CziLight_ControlLight(char arg)  //0 1  2 3
// {
//     USER_LOG_INFO("11Set light type is packet[1] is %d\n", arg);
//     USER_LOG_INFO("22Set light type is packet[1] is %x\n", arg);
//     printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
//     T_ProtRspData rsp;
//     // unsigned char Val[2];
//     // int len = 0;
//     // memset(Val, 0x00, 2);
//     // if(arg == 0x00) {
//     //     Val[0] = 0xA1;
//     //     len = 1;
//     // }else {
//     //     Val[0] = 0xA0,
//     //     Val[1] = arg - 1;
//     //     len = sizeof(Val);
//     // }
//     unsigned char packet[1] = {0};
//     // packet[0] = 0x63;
//     packet[0] = (unsigned char)arg;
//     int len = sizeof(packet);

//     /* TODO packet protocol */
    
//     int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, packet, len, &rsp);
    
//     /* set queue (psdk-->prot) */
//     CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);

//     USER_LOG_INFO("Set light type is ok!");
//     USER_LOG_INFO("Set light type is packet[1] is 0x%02x\n", packet[0]);
//     // USER_LOG_INFO("Set light type is packet[1] is 0x%02x\n", packet[1]);
//     // USER_LOG_INFO("Set light type is packet[1] is %x\n", packet);
//     USER_LOG_INFO("Set light type is packet[1] is %d\n", len);
//     return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
// }


T_DjiReturnCode CziLight_ControlLight(char arg)
{

    printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__); 
    T_ProtRspData rsp;

    unsigned char packet[2];
    int len = 0;
    memset(packet, 0x00, 3);
    if(arg == 0x00) {
        packet[0] = 0xA1;
        packet[1] = 0;
        len = 2;
    }else {
        packet[0] = 0xA0;
        packet[1] = (unsigned char)arg - 1;
    }

    len = sizeof(packet);


    printf("CziLight_ControlLight lightmode:%x  lighttype:%x", packet[0], packet[1]);  

    /* TODO packet protocol */
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, (unsigned char*)packet, len, &rsp);
    
    /* set queue (psdk-->prot) */
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode CziLight_BreathingLight(char arg)
{
    printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__); 
    T_ProtRspData rsp;

    unsigned char packet[2];
    int len = 0; 
    unsigned char input_data[2] = {0x00};
    input_data[0] = ALARM_LIGHT_BREATHING_LIGHT;
    input_data[1] = (unsigned char)arg;
    // len = sizeof(packet);

    printf("!!!!!!!!!!!!!!psdk send  light cmd:%x  mode:%x\n", input_data[0], input_data[1]);
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, (unsigned char*)input_data, sizeof(input_data), &rsp);
    
    /* set queue (psdk-->prot) */
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
