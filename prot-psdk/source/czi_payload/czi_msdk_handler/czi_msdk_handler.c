#include "dji_logger.h"
#include "dji_platform.h"
#include "dji_error.h"
#include "dji_low_speed_data_channel.h"

#include "../czi_transmission/czi_transmission.h"
#include "../czi_protocol_handler/protocol_longFormatHandler.h"
#include "czi_msdk_handler.h"
#include "../../psdk_config.h"
#include "msdk_protobuf.pb-c.h"
#include "../czi_megaphone/czi_megaphone.h"
#include <sys/statvfs.h>
#include <errno.h> 
#include <string.h>
#include "../czi_psdk_handler/czi_psdk_handler.h"
#include "../../dji_module/widget/widget.h"
#include "../czi_json_handler/czi_json_handler.h"
#include "../czi_json_handler/cJSON/czi_JSON.h"

static char Ptb_SendMsdkBuf[256] = {0};
static T_DjiTaskHandle gs_threadId_SendToMsdkHandler;
static T_DjiTaskHandle gs_threadId_RecvFromMsdkHandler;
static FILE *Mp3FilePtr = NULL;
static char Mp3FileName[MAXLEN_FILE_NAME + 1 ] = "";

static void *MsdkTask_RecvQueueDataFromMsdk(void *arg);
static int CziMsdkHandler_SwitchDevice();
static int CziMsdkHandler_MediaInfo();
static int CziMsdkHandler_TtsPlay(char *input_data, int len);
static int CziMsdkHandler_RecordPlay(char *input_data, int len);
static int CziMsdkHandler_RecordStream(char *input_data, int len);
static void CziMsdkHandler_CloseMp3File();
static int CziMegaphone_ReadJsonFile(const char* file_path, char** content);
static int CziMegaphone_SendJsonContent(const char* content);
static void CziMegaphone_ProcessCommand();
static int CziMsdkHandler_UpdateModelTTSContent(char* filePath, int index, char* newContent);
static int CziMsdkHandler_UpdateContent(czi_JSON* root, int index, const char* newContent);
static int CziMsdkHandler_UpdateCurrentModel(unsigned char* rspData, int dataSize);
static int CziMsdkHandler_UpdateCurrentModelArray(czi_JSON* root, unsigned char* rspData, int dataSize);

#define MAX_CHUNK_SIZE 89

static T_DjiReturnCode MsdkHandler_SendData(const uint8_t *data, uint16_t len) 
{
    T_DjiReturnCode djiStat = 0;
    E_DjiChannelAddress channelAddress;
        // printf("send data to msdk\n");
        // for(int i = 0; i < 8; i++)
        //     printf("%02X ",data[i]);
        // printf("\n");
        

    if(len <= MAXLEN_SEND_TO_MASK) {
        channelAddress = DJI_CHANNEL_ADDRESS_MASTER_RC_APP;
        djiStat = DjiLowSpeedDataChannel_SendData(channelAddress, data, (uint8_t)len + 1);
        if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) 
            USER_LOG_ERROR("send data to mobile error.");
    } 
    else {
        USER_LOG_ERROR("data send to MSDk over 127");
    }
    return djiStat;
}

static void MsdkHandler_SendDataToMsdk(const uint8_t *data, uint16_t len)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_DjiReturnCode djiStat;
    int timeoutCount = 0;

#if DEBUG_PRINTF
    if (len != 27)
    {
        printf("send data to msdk\n");
        for(int i = 0; i < 8; i++)
            printf("%02X ",data[i]);
        printf("\n");
    }
#endif
    while(1){
       
        djiStat = MsdkHandler_SendData(data, len);
        if(djiStat == DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS)
            break;
        // osalHandler->TaskSleepMs(5);
        timeoutCount++;
        if(timeoutCount > MAXNUM_TIMEOUT_COUNT){
            USER_LOG_ERROR("send data to msdk error with times:%d", timeoutCount);
            break;
        }
    }
}


static void *MsdkTask_SendQueueDataToMsdk(void *arg)
{
    T_DjiReturnCode djiStat, cziStat;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_CziQueueQueue queue =  { 0x00 };
    // T_CziProtRspData prot = {};

    int ret;
    while (1) {
        int sentLen = 0;
        int offset  = 0;
        ret = CziTransmission_GetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, &queue);
        if (queue.qdata.len <= MAXLEN_SEND_TO_MASK) {
            MsdkHandler_SendDataToMsdk(queue.qdata.mtext, queue.qdata.len);
        } 
        else {
            for (sentLen = queue.qdata.len; sentLen > MAXLEN_SEND_TO_MASK; sentLen -= MAXLEN_SEND_TO_MASK) {
                MsdkHandler_SendDataToMsdk(queue.qdata.mtext + offset, MAXLEN_SEND_TO_MASK);
                offset += MAXLEN_SEND_TO_MASK;
            }
            MsdkHandler_SendDataToMsdk(queue.qdata.mtext + offset, sentLen);
        }
        // osalHandler->TaskSleepMs(5);
    }
}

static void CziMsdkHandler_UpdateProtobuf(char *ptb, uint16_t *ptbLen)
{
    CziProtobufInfo protobuf = {};
    czi_protobuf_info__init(&protobuf);

    // protobuf.volume = 20;

    *ptbLen = czi_protobuf_info__pack(&protobuf, ptb);

}

static void CziMsdkHandler_Packdata(char *ptb, uint16_t ptbLen, uint16_t *bufferlen)
{
    memset(Ptb_SendMsdkBuf, 0x00, sizeof(Ptb_SendMsdkBuf));
    *bufferlen = ptbLen + 6;
    Ptb_SendMsdkBuf[0] = 0x24;
    Ptb_SendMsdkBuf[1] = (*bufferlen | 0xFF);
    Ptb_SendMsdkBuf[2] = (*bufferlen >> 8 | 0xFF) ;
    Ptb_SendMsdkBuf[3] = 0x01;
    Ptb_SendMsdkBuf[*bufferlen - 1] = 0x23;
    memcpy(Ptb_SendMsdkBuf+4, ptb, ptbLen);

}

//Switching devices
static int CziMsdkHandler_SwitchDevice()
{
    char protobuf[256];
    int16_t proLen = 0;
    int16_t bufferlen = 0;
    int ret = 0;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    memset(protobuf, 0x00, sizeof(protobuf));
    //1.update protobuf
    CziMsdkHandler_UpdateProtobuf(protobuf, &proLen);
    //2.package data
    if(proLen > 0) {
        CziMsdkHandler_Packdata(protobuf, proLen, &bufferlen);
        USER_LOG_INFO("proto data is pack");
    //3.send to msdk
        ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, protobuf, bufferlen);
    }
    
    return ret;
}

T_DjiReturnCode CziMsdkHandler_Init(void)
{
    T_DjiReturnCode cziStat, returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    // CziTransmission_RingQueueInit(RingQInfo);
    if (osalHandler->TaskCreate("czi pilot send data to msdk", MsdkTask_SendQueueDataToMsdk, STACK_SIZE_CZI_PILOT_HANDLER, NULL, 
                                &gs_threadId_SendToMsdkHandler) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("MsdkTask_SendQueueDataToMsdk create error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
    
    if (osalHandler->TaskCreate("msdk send data to czi pilot", MsdkTask_RecvQueueDataFromMsdk, STACK_SIZE_CZI_PILOT_HANDLER, NULL, 
                                &gs_threadId_RecvFromMsdkHandler) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("MsdkTask_RecvQueueDataFromMsdk create error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }


    USER_LOG_INFO("CziMsdkHandler_TaskInit");

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static void *MsdkTask_RecvQueueDataFromMsdk(void *arg)
{
    
    T_CziQueueQueue queue =  {0x00};
    T_ProtRspData prot = {};
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    int ret;
    int model_index;
    int receivedTextDataIndex = 0;
    bool receivingTextData = false;
    char receivedTextData[256];
    while(1) {
        osalHandler->TaskSleepMs(5);
        ret = CziTransmission_GetQueueDataById(QUEUE_TYPE_MSDK_TO_PSKD, &queue);
        
        Prot_LongFormatUnpack(queue.qdata.mtext, queue.qdata.len, &prot);
        // printf("1:%x   2:%x    3:%x   4:%x  5:%x   6:%x   7%x  8:%x\n", prot.protCmd, prot.rspData[0],  prot.rspData[1], prot.rspData[2], prot.rspData[3], prot.rspData[4],prot.rspData[5], prot.rspData[6],prot.rspData[7]);
       //43  f2      // 2D  ff  f3  28  c4... 
        switch (prot.protCmd)
        {

        case CZI_PROTOCOL_COMMON_CMD_DEVICE_VOLUME: 
        {
             struct statvfs st;
            if (statvfs(STORAGE_PATH, &st) != 0) {
                printf("statvfs error: %s\n", strerror(errno));
                break;
            }
            unsigned long long TotalMemory = st.f_frsize * st.f_blocks; 
            unsigned long long AvailableMemory = st.f_frsize * st.f_bavail;
            double TotalMemory_mb = TotalMemory / (1024.0 * 1024);
            double AvailableMemory_mb = AvailableMemory / (1024.0 * 1024);

            char result[BUFFER_SIZE];
            snprintf(result, BUFFER_SIZE, "%.1f/%.1f", AvailableMemory_mb, TotalMemory_mb);

            
            uint8_t buffer[BUFFER_SIZE + 1]; 
            strncpy((char *)&buffer[0], result, BUFFER_SIZE);
            
            T_ProtRspData rsp;
            int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_DEVICE_VOLUME, (char *)buffer,strlen(buffer), &rsp);
            CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
            break;
        }
        case CZI_PROTOCOL_COMMON_CMD_RECORD_CACHE_STREAM:  //0x2D  录音流传输
        {
            CziMsdkHandler_RecordStream(prot.rspData, prot.rspLen);
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_TTS_PLAY:/*tts 0x10*/
        {
            CziMsdkHandler_TtsPlay(prot.rspData, prot.rspLen);
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_TTS_PLAY_STOP:  //0x13 TTS停止播放
        {
            unsigned char input_data[2];
            T_ProtRspData rsp = {0x00};
            input_data[0] = MEDIA_CHANNEL_TYPE_MASTER;
            input_data[1] = 0xF3;
            int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_STATE, (unsigned char *)input_data, sizeof(input_data), &rsp);
            CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_TTS_PLAY_STATE:
        {
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_MEDIA_LIST: /*media info file list 0x3c*/
        {
            CziMsdkHandler_MediaInfo();
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_FILE_MODIFY:/*0x31*/
        {
            int index = prot.rspData[1];
            index--;
            char *Newfilename = (char *)&prot.rspData[2];
            char buffer[MAXLEN_FILE_NAME + 2];
            buffer[0] = DIR_TYPE_MUSIC;
            buffer[1] = index;
            strcpy((char *)&buffer[2], Newfilename);
            T_ProtRspData rsp;
            int bufferlen = strlen(Newfilename) + 2;
            int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_FILE_MODIFY, (unsigned char *)buffer, bufferlen, &rsp);
            CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
            break;
        }
        case CZI_PROTOCOL_COMMON_CMD_FILE_DELETE:
        {
            prot.rspData[1]--;
            T_ProtRspData rsp;
            int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_FILE_DELETE, (unsigned char *)prot.rspData, 2, &rsp);
            CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
            break;
        }
        case CZI_PROTOCOL_COMMON_CMD_FILE_SAVE:/*0x33*/
        {
            printf("Here is save place!\n");
            if(prot.rspData[0] == 0xF2){
                if(Mp3FilePtr != NULL){
                    CziMsdkHandler_CloseMp3File();
                }
                char filename[MAXLEN_FILE_NAME + 1];
                snprintf(filename, MAXLEN_FILE_NAME + 1, "/czzn/cusr/music/%s", &prot.rspData[1]);
                Mp3FilePtr = fopen(filename, "wb");
                if (Mp3FilePtr == NULL)
                {
                    printf("Failed to create %s\n", filename);
                }
                else
                {
                    strcpy(Mp3FileName, filename);
                }
                T_ProtRspData rsp = {0x00};
                int totalen = Prot_LongFormatPack(0x00, NULL, 0, &rsp);
                CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
            }
            if(prot.rspData[0] == 0xF3){
                CziMsdkHandler_CloseMp3File();
                T_ProtRspData rsp = {0x00};
                int totalen = Prot_LongFormatPack(0x00, NULL, 0, &rsp);
                CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
            }
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_FILE_STREAM:/*0x3D*/
        {
            if(Mp3FilePtr != NULL){
                printf("Succeed to fwrite mp3\n");
                fwrite(prot.rspData,sizeof(unsigned char),prot.rspLen,Mp3FilePtr);
            }
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_FILE_HEAD: //0x34
        {
            int index = prot.rspData[1];
            index--;
            uint8_t buffer[MAXLEN_FILE_NAME + 2];
            buffer[0] = DIR_TYPE_MUSIC;
            buffer[1] = index;
            T_ProtRspData rsp;
            int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_FILE_HEAD, buffer, 2, &rsp);
            CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_RECORD_CACHE_PLAY:   //0x43
        {
            CziMsdkHandler_RecordPlay(prot.rspData, prot.rspLen);
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_MUSIC_PLAY:  //0x21  
        {
            T_MusicListInfo MusicListInfo;
            // DIR_TYPE_MUSIC = DIR_TYPE_MUSIC;
            CziMegaphone_GetMusicSum(&MusicListInfo);
            CziMegaphone_GetMusicName(&MusicListInfo, prot.rspData[2]);
            T_ProtRspData rsp = {0x00};
            // input_data[2] = gs_RecordFilename;
            snprintf(&prot.rspData[2], MAXLEN_FILE_NAME, "%s", MusicListInfo.musicName);
            osalHandler->TaskSleepMs(20);
            int len = strlen(MusicListInfo.musicName);
            int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_PLAY, (unsigned char *)prot.rspData, len + 2, &rsp);
            CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
            break;
        }
        case CZI_PROTOCOL_COMMON_CMD_RECORD_PLAY_STATE:  //0x40  录音播放
        {
            for(int i=0;i<prot.rspLen;i++){
                printf("prot.rspData[%d] is %x\n",i,prot.rspData[i]);
            }
            if(prot.rspData[0] == 0XF0){
                CziMegaphone_StopPlay();
            }
            // goto SendToProt;
            break;
        }
        case CZI_PROTOCOL_COMMON_CMD_TTS_PLAY_MODE:
        case CZI_PROTOCOL_COMMON_CMD_RECORD_MODE:
        case CZI_PROTOCOL_COMMON_CMD_MUSIC_MODE:  //0x22 音乐播放模式
        {
            unsigned char input_data[2] = {0x00};
            input_data[0] = 0x00;
            input_data[1] = prot.rspData[0];
            T_ProtRspData rsp;
            int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_MODE, (unsigned char *)input_data, sizeof(input_data), &rsp);
            
            // printf("rsplen is %d, rspdata is :", rsp.rspLen);
            // for (int i = 0; i < rsp.rspLen; i++)
            // {
            //     printf("0x%x ", rsp.rspData[i]);
            // }
            // printf("\n");

            /* set queue (psdk-->prot) */
            CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
            break;
        }
        case CZI_PROTOCOL_COMMON_CMD_STARTUP_MUTE:
        {
           char value = 0x00;
            if (prot.rspData[0] == 0xf1) {
                value = 0x01;
            }
            CziMegaphone_StartBroadcast(value);
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_LIGHT:
        {
            printf("!!!!!!!!!!MSDK  SEND    1:%d   2:%d  3:%d\n", prot.rspData[0], prot.rspData[1], prot.rspData[2]);
            if(prot.rspData[0] == 0xA0 || prot.rspData[0] == 0xA1)
            {
                // unsigned char packet[2];
                // int len = 0;
                // memset(packet, 0x00, 3);
                // if(arg == 0x00) {
                //     packet[0] = 0xA1;
                //     packet[1] = 0;
                //     len = 2;
                // }else {
                //     packet[0] = 0xA0;
                //     packet[1] = (unsigned char)arg-1;
                // }

                // len = sizeof(packet);
                T_ProtRspData rsp;
                unsigned char input_data[2] = {0x00};
                input_data[0] = prot.rspData[0];
                input_data[1] = prot.rspData[1];
                
                
                int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_LIGHT, (unsigned char*)input_data, sizeof(input_data), &rsp);
                CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
            }
            if (prot.rspData[0] == 0xA3) {
                CziMegaphone_SetBreathLight(prot.rspData[1]);
            }
            break;
        }

      
        // case CZI_PROTOCOL_COMMON_CMD_AI_VIDEO:
        // {
        //     for(int i=0;i<prot.rspLen;i++){
        //         printf("CZI_PROTOCOL_COMMON_CMD_AI_VIDEO--prot.rspData[%d] is %x\n",i,prot.rspData[i]);
        //     }
        //     printf("prot.rsplen is %d\n",prot.rspLen);
        //     if(prot.rspData[0] == 0x00){
        //         CziWidget_SetAiVideoStream(prot.rspData[1]);
        //         if (prot.rspData[1] == 0xF0 || prot.rspData[1] == 0xF2 || prot.rspData[1] == 0xF3){
        //             CziMegaphone_CtrlMusicStop();
        //         }
        //         T_ProtRspData rsp = {0x00};  
        //         unsigned char input_data[4];
        //         input_data[0] = prot.rspData[0];
        //         input_data[1] = prot.rspData[1];
        //         input_data[2] = 0x00;
        //         int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_AI_VIDEO, (unsigned char*)input_data, sizeof(input_data), &rsp);
        //         ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
        //     }
        //     else if (prot.rspData[0] == 0x02){
        //         if (prot.rspData[2] == 0xF1 && prot.rspData[1] == 0x01){
        //             model_index = prot.rspData[3];
        //         }
        //         else if (prot.rspData[2] == 0xF2){
        //             receivingTextData = true;
        //             int dataLength = prot.rspLen - 3;
        //             if (receivedTextDataIndex + dataLength < sizeof(receivedTextData)) {
        //                 memcpy(receivedTextData + receivedTextDataIndex, prot.rspData + 3, dataLength);
        //                 receivedTextDataIndex += dataLength;
        //             }
        //             printf("receivedTextData is %s\n",receivedTextData);
        //         }
        //         else if (prot.rspData[2] == 0xF3 && prot.rspData[1] == 0x01){
        //             if (receivingTextData) {
        //                 receivingTextData = false;
        //                 receivedTextData[receivedTextDataIndex] = '\0';
        //                 CziMsdkHandler_UpdateModelTTSContent(FILEPATH_MODEL_CONFIG_JSON,model_index, receivedTextData);
        //                 receivedTextDataIndex = 0; 
        //                 }
        //         } 
        //     }
        //     else if (prot.rspData[0] == 0x03) {
        //         CziMsdkHandler_UpdateCurrentModel(prot.rspData, prot.rspLen);
        //     }
        //     else if (prot.rspData[0] == 0x04 && prot.rspLen == 1){
        //         int videoStreamFlag;
        //         CziWidget_GetIndexValue(WIDGET_INDEX_LIGHT_SPOT_AI, &videoStreamFlag);
        //         T_ProtRspData rsp = {0x00};   
        //         unsigned char input_data[2];
        //         input_data[0] = prot.rspData[0];
        //         input_data[1] = (videoStreamFlag == 1) ? 0xF1 : 0xF3;
        //         int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_AI_VIDEO, (unsigned char*)input_data, sizeof(input_data), &rsp);
        //         ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
        //     }


        //     break;
        // }
        case CZI_PROTOCOL_COMMON_CMD_CONFIG_INFO:
        {
            T_ProtRspData rsp = {0x00};
            unsigned char input_data[1];  
            input_data[0] = 0xF1;
            Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_CONFIG_INFO, (unsigned char*)input_data, sizeof(input_data), &rsp);
            ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
            // CziMegaphone_ProcessCommand();
            input_data[0] = 0xF3;
            Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_CONFIG_INFO, (unsigned char*)input_data, sizeof(input_data), &rsp);
            ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
            break;
        }

        case CZI_PROTOCOL_COMMON_CMD_LIGHTANDVOLUME: /*One click siren*/
        {
            
        }



        default:
            goto SendToProt;

SendToProt:
        printf(" dji send to prot, cmd is 0x%x\n", prot.protCmd);
        CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, queue.qdata.mtext, queue.qdata.len);
        }
    }
}
T_MusicListInfo gMusicListInfo;
static int CziMsdkHandler_MediaInfo()
{
    T_ProtRspData rsp;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    // DIR_TYPE_MUSIC = DIR_TYPE_MUSIC;
    CziMegaphone_GetMusicSum(&gMusicListInfo);
    unsigned char packet[2];
    packet[0] = 0xF1;
    packet[1] = (unsigned char)gMusicListInfo.sum;
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MEDIA_LIST, (unsigned char *)packet, 2, &rsp);
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
    osalHandler->TaskSleepMs(10);

    char i = 1;
    FILE *fp = fopen(FILEPATH_MUSIC_LIST, "r");
    if(fp == NULL) {
        printf("(%s %s LINE-%d)\n", __FILE__, __FUNCTION__, __LINE__);
        printf("open %s failed!\n", FILEPATH_MUSIC_LIST);
        return -1;
    }
    char musicname[MAXLEN_FILE_NAME];
    char inputdata[MAXLEN_FILE_NAME + 2];
    while(!feof(fp)) {
        osalHandler->TaskSleepMs(10);
        memset(musicname, 0x00, MAXLEN_FILE_NAME);
        fgets(musicname, MAXLEN_FILE_NAME, fp);
        int len = strlen(musicname) - 1; /*unnecessary '\r'*/

        if(len != -1) {
            memset(inputdata, 0x00, MAXLEN_FILE_NAME + 2);
            inputdata[0] = 0xF2;
            inputdata[1] = i;
            if(len > MAXLEN_FILE_NAME)
                len = MAXLEN_FILE_NAME;
            
            snprintf(&inputdata[2], len + 1, "%s", musicname);

            totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MEDIA_LIST, (unsigned char *)inputdata, len + 3, &rsp);/* add \0*/
            CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
            i++;
        }
    }
    fclose(fp);
    printf("send Media info is ok\n");

    return 0;
    
}

static int CziMsdkHandler_TtsPlay(char *input_data, int len)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    switch (input_data[0])
    {
    case 0xF1:
    {
        T_ProtRspData rsp;
        char ttsType;
        char newvoicetype;
        CziMegaphone_SetTtsVoiceType(input_data[1], &ttsType, &newvoicetype);
        CziMegaphone_SetTtsSpeed(input_data[2]);
        input_data[3] = ttsType;
        char inputdata[4];
        inputdata[0] = input_data[0];
        inputdata[1] = ttsType;
        inputdata[2] = newvoicetype;
        inputdata[3] = input_data[2];
        printf("1:%x   2:%x  3：%x   4:%x\n", inputdata[0], inputdata[1], inputdata[2], inputdata[3]);
        printf("newvoicetype is %x\n", newvoicetype);
        int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY, (unsigned char*)inputdata, len + 1, &rsp);
        CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
        break;
    }
    
    case 0xF2:
    {
        T_ProtRspData rsp;
        int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY, (unsigned char*)input_data, len, &rsp);
        CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
        break;
    }

    case 0xF3:
    {
        osalHandler->TaskSleepMs(400);
        T_ProtRspData rsp;
        int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY, (unsigned char*)input_data, len, &rsp);
        CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
        break;
    }

    default:
        break;
    }
}
#define FILENAME_CACHE_MP3      "cache.mp3"
static FILE *fp = NULL;
static int CziMsdkHandler_RecordPlay(char *input_data, int len)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    
    if(input_data[0] == 0xF2) {
        char pathFile[MAXLEN_FILEPATH] = {0x00};
        snprintf(pathFile, MAXLEN_FILEPATH, "/czzn/cusr/music/%s", FILENAME_CACHE_MP3);
        fp = fopen(pathFile, "wb");
        if(fp == NULL) {
            printf("open %s failed!\n", pathFile);
            return -1;
        }
    }else {
        fclose(fp);
        T_ProtRspData rsp = {0x00};
        unsigned char inputdata[MAXLEN_FILE_NAME];
        inputdata[0] = 0x00;
        inputdata[1] = 0x00;
        // input_data[2] = gs_RecordFilename;
        snprintf(&inputdata[2], MAXLEN_FILE_NAME, "%s", FILENAME_CACHE_MP3);
        osalHandler->TaskSleepMs(30);
        int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_MUSIC_PLAY, (unsigned char *)inputdata, sizeof(inputdata), &rsp);
        CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    }
}

static int CziMsdkHandler_RecordStream(char *input_data, int len)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    char pathFile[MAXLEN_FILEPATH] = {0x00};
    snprintf(pathFile, MAXLEN_FILEPATH, "/czzn/cusr/music/%s", FILENAME_CACHE_MP3);
    // osalHandler->TaskSleepMs(50); 
    fwrite(input_data, sizeof(char), len, fp);
    // osalHandler->TaskSleepMs(50); 
    fflush(fp);
}


static void CziMsdkHandler_CloseMp3File()
{
    if(Mp3FilePtr != NULL)
    {
        fclose(Mp3FilePtr);
        Mp3FilePtr = NULL;
        printf("Closed %s\n",Mp3FileName);
        CziMegaphone_UpdateMusicList(Mp3FileName);
    }
}
T_DjiReturnCode CziMegaphone_UpdateMusicList(const char *filename)
{
    T_ProtRspData rsp = {0x00};
    unsigned char input_data[MAXLEN_FILE_NAME + 3];
    input_data[0] = 0x00;
    input_data[1] = DIR_TYPE_MUSIC;
    snprintf((char *)&input_data[2], MAXLEN_FILE_NAME, "%s", filename);
    int totalen = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_FILE_ADD, input_data, sizeof(input_data), &rsp);
    CziTransmission_SetQueueDataById(QUEUE_TYPE_PSKD_TO_PROT, rsp.rspData, rsp.rspLen);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static int CziMegaphone_ReadJsonFile(const char* file_path, char** content) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        printf("Failed to open file: %s", file_path);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *content = (char*)malloc(file_size + 1);
    if (!*content) {
        printf("Failed to allocate memory");
        fclose(file);
        return -1;
    }

    fread(*content, 1, file_size, file);
    (*content)[file_size] = '\0';
    fclose(file);

    return 0;
}

static int CziMegaphone_SendJsonContent(const char* content) {
    unsigned char input_data[90];
    const char* ptr = content;
    size_t remaining_len = strlen(content);
    int ret;

    while (remaining_len > 0) {
        size_t chunk_len = remaining_len > MAX_CHUNK_SIZE ? MAX_CHUNK_SIZE : remaining_len;

        input_data[0] = 0xF2;
        memcpy(&input_data[1], ptr, chunk_len);

        T_ProtRspData rsp = {0x00};
        int total_len = Prot_LongFormatPack(CZI_PROTOCOL_COMMON_CMD_CONFIG_INFO, input_data, chunk_len + 1, &rsp);
        ret = CziTransmission_SetQueueDataById(QUEUE_TYPE_PILOT_TO_MSDK, rsp.rspData, rsp.rspLen);
        if (ret != 0) {
            printf("Failed to send data to queue");
            return ret;
        }

        ptr += chunk_len;
        remaining_len -= chunk_len;
    }

    return 0;
}

static void CziMegaphone_ProcessCommand() {
    int ret;
    char* json_content;

    ret = CziMegaphone_ReadJsonFile(FILEPATH_MODEL_CONFIG_JSON, &json_content);
    if (ret != 0) {
        printf("Failed to read JSON file");
        return;
    }
    ret = CziMegaphone_SendJsonContent(json_content);
    if (ret != 0) {
        printf("Failed to send JSON content");
    }

    free(json_content);
}

static int CziMsdkHandler_UpdateModelTTSContent(char* filePath, int index, char* newContent) {
    if (CziJsonHandler_Open(filePath, 0) != 0) {
        return -1;
    }

    czi_JSON* root = CziJsonHandler_GetRoot();
    if (!root) {
        CziJsonHandler_Close(0);
        return -1;
    }

    int result = CziMsdkHandler_UpdateContent(root, index, newContent);

    if (CziJsonHandler_Close(1) != 0) {
        return -1; 
    }

    return result; 
}

static int CziMsdkHandler_UpdateContent(czi_JSON* root, int index, const char* newContent) {
    czi_JSON* model_tts = czi_JSON_GetObjectItem(root, "model_tts");
    if (!model_tts || !czi_JSON_IsArray(model_tts)) {
        return -1; 
    }

    czi_JSON* item = czi_JSON_GetArrayItem(model_tts, index);
    if (!item || !czi_JSON_IsObject(item)) {
        return -1; 
    }

    czi_JSON* content = czi_JSON_GetObjectItem(item, "content");
    if (!content) {
        czi_JSON_AddStringToObject(item, "content", newContent);
    } else {
        czi_JSON_ReplaceItemInObject(item, "content", czi_JSON_CreateString(newContent));
    }

    return 0;
}

static int CziMsdkHandler_UpdateCurrentModel(unsigned char* rspData, int dataSize) {
    if (CziJsonHandler_Open(FILEPATH_MODEL_CONFIG_JSON, 0) != 0) {
        return -1;
    }

    czi_JSON* root = CziJsonHandler_GetRoot();
    if (!root) {
        CziJsonHandler_Close(0);
        return -1;
    }

    int result = CziMsdkHandler_UpdateCurrentModelArray(root, rspData, dataSize);

    if (CziJsonHandler_Close(1) != 0) {
        return -1; 
    }

    return result; 
}

static int CziMsdkHandler_UpdateCurrentModelArray(czi_JSON* root, unsigned char* rspData, int dataSize) {
    czi_JSON* current_model = czi_JSON_GetObjectItem(root, "current_model");
    if (!current_model || !czi_JSON_IsArray(current_model)) {
        return -1; 
    }
    czi_JSON_DeleteItemFromObject(root, "current_model");
    current_model = czi_JSON_CreateArray();
    char hexStr[3] = {0}; 
    for (int i = 1; i < dataSize; ++i) {
        snprintf(hexStr, sizeof(hexStr), "%02x", rspData[i]);
        int value;
        if (sscanf(hexStr, "%x", &value) != 1) {
            return -1;
        }
        czi_JSON_AddItemToArray(current_model, czi_JSON_CreateNumber(value));
    }
    czi_JSON_AddItemToObject(root, "current_model", current_model);
    return 0;
}
