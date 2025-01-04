#include <stdio.h>

#include "czi_protocol_handler.h"

static T_CziProtHandler gs_tCziProtHandler = {0x00};

// int CziProtocolHandler_UnpackData(unsigned char *data, int len, T_CziProtRspData *unpack)
// {
//     /**
//      * @FIXME
//      * The bt's receiving procedure fixs the validation of data, so that
//      * no need for validating on the data transfering to be here.
//      * But here it is based on uart.
//      */
//     unsigned char xor = 0;
//     int real_len = 0;

//     /* validate all things here */
//     if (len < PLF_LENGTH_EXCEPT)
//         return -1;

//     if (PLF_HEADER_DATA != data[0])
//         return -1;

//     real_len = data[1] * 256 + data[2];
//     if (len != real_len)
//         return -1;

// #if 0
//     xor = CziPlf_ValidateXOR(&data[3],len - PLF_LENGTH_EXCEPT);
//     if (xor != data[len - 2]) {
//         return -1;
//     }
// #endif 
//     if (PLF_TAILER_DATA != data[len - 1])
//         return -1;

//     memcpy(unpack->rsp_data,&data[4],len - PLF_LENGTH_EXCEPT);
//     unpack->rsp_len = len - PLF_LENGTH_EXCEPT;
//     unpack->plf_cmd = data[3];
//     return 0;
// }

int CziProtocolHandler_Init(void)
{
    T_CziProtConfiguration tProtLongCfg = {
        .protDataChannel = 1,
        .type = PROTOCOL_LONG_FORMAT,
        .sendCallback = NULL
    };

    int ret = CziProt_ConfigurationInit(&tProtLongCfg, &gs_tCziProtHandler);

    return 0;
}