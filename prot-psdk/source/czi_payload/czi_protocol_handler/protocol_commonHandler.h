/**
 ********************************************************************
 * @author yealkio
 * @file    protocol_commonHandler.h
 * @version V1.0.0
 * @date    2024/01/01
 * @brief   This file is to indicate external communication with devices.
 * @attention Code file rules:
 * rule: file encoding use UTF8;
 * rule: max line length 120 characters;
 * rule: line separator \r\n;
 * rule: use clion auto code format tool.
 */
#ifndef _PROTOCOL_COMMON_HANDLER_H_
#define _PROTOCOL_COMMON_HANDLER_H_

#include <stdbool.h>

#define   KEY_CMD_COMOON    (0x01 << 0)
#define   KEY_CMD_MEDIA     (0x01 << 1)
#define   KEY_CMD_FILE      (0x01 << 2)

typedef enum __record_rsp_status
{
    PROT_HAVE_NO_WORK = 0xC0,       /* no work */
    PROT_HAVE_RESPONSED = 0xC1,   /* have responsed */
    PROT_HAVE_STARTED   = 0xC2,   /* have started*/

    /* TBD */
}E_ProtRspStatus;

typedef enum
{
    PROTOCOL_LONG_FORMAT = 0x00,
    PROTOCOL_SHORT_FORMAT,

}E_Protocol_Type;

typedef enum
{
    PROT_PALY_STATUS_STOP =      0xF0,
    PROT_PLAY_STATUS_PLAYING =   0xF1,
    PROT_PLAY_STATUS_PAUSE =     0xF2

    /* TBD */
} E_ProtCodePlayStatus;

/* circle mode */
typedef enum
{
    PROT_CIRCLE_SINGLE_ONCE =    0xC1,
    PROT_CIRCLE_SINGLE_CIRCLE =  0xC2,
    PROT_CIRCLE_LIST_ONCE =      0xC3,
    PROT_CIRCLE_LIST_CIRCLE =    0xC4,
    PROT_CIRCLE_RANDOM =         0xC5

    /* TBD */
} E_ProtCodeCircleMode;

/* light mode */
typedef enum
{
    PROT_LIGHT_TURN_OFF_MODE     = 0xA0,
    PROT_LIGHT_RRBB_MODE,
    PROT_LIGHT_RRBB_FAST_MODE,
    PROT_LIGHT_RBRB_MODE,
    PROT_LIGHT_RBRB_FAST_MODE

    /* TBD */
} E_ProtCodeLightMode;

/* stream state */
typedef enum
{
    PROT_STREAM_START = 0xF2,
    PROT_STREAM_STOP = 0xF3,
    //PROT_STREAM_BUSY = 0x03
    
    /* TBD */
} E_ProtCodeStreamStatus;

/* return state */
typedef enum
{
    PROT_RETURN_SUCCESS = 0x00,
    PROT_RETURN_FAIL = 0x01,
    PROT_RETURN_BUSY = 0x03

    /* TBD */
} E_ProtCodeReturnStatus;

/* file list state */
typedef enum
{
    PROT_FILE_LIST_NONE = 0xF0,
    PROT_FILE_LIST_COMPLETELY,
    PROT_FILE_LIST_CONTINOUS,
    PROT_FILE_LIST_LAST
} E_ProtFileLIstStatus;

//1024 for ymodem, 200 for protocol and reserved
#define PACKET_MAX_LEN (1024 + 200) 
#define LEN_CMD_MAX     PACKET_MAX_LEN
#define BUFFER_SZIE     LEN_CMD_MAX
#define WORKSPACE_SIZE  BUFFER_SZIE * 2

/**
 * @brief The protocol channal work space.
 */
typedef struct {
    unsigned char lastBufData[BUFFER_SZIE]; /* last buffer of the data */
    int  lastBufLen;               /* last len of buffer */
    unsigned char workspace[WORKSPACE_SIZE];/* work space for being handled */
    int  workLen;                  /* length of work space's buffer */
    unsigned char *workPtr;                 /* pointer of work space */
    int AntiDataDoneLeft; 
} T_ProtCommonWorkSpace;

typedef int(*pProtocolResponseFunc)(unsigned char *data, int len);

#define __PACKET_CMD     \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_HANDSHAKE,                      0x01)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_HEARTBEAT,                      0x02)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_SERVER_LOGIN,                   0x03)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_ASK_INFO,                       0x05)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_SEND_SN,                        0x06)        \
        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY,                       0x10)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_TTS_CHANGE_STAT,                0x11)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY_MODE,                  0x12)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY_STOP,                  0x13)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY_STATE,                 0x14)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_TTS_PLAY_END,                   0x15)        \
        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_MUSIC_STATE,                    0x20)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_MUSIC_PLAY,                     0x21)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_MUSIC_MODE,                     0x22)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_MUSIC_VOL,                      0x24)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_STREAM_UP,                      0x29)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_DATA_STREAM_UP,                     0x2A)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_STREAM_DOWN,                    0x2B)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_DATA_STREAM_DOWN,                   0x2C)        \
        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_ADD,                       0x30)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_MODIFY,                    0x31)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_DELETE,                    0x32)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_SAVE,                      0x33)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_HEAD,                      0x34)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_DEL_ALL,                   0x35)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_WRITE,                     0x36)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_CREATE,                    0x37)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_MEDIA_LIST,                     0x3C)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_STREAM,                    0x3D)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_CONFIG_INFO,                    0x3F)        \
        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_RECORD_PLAY_STATE,              0x40)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_RECORD_PLAY,                    0x41)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_RECORD_MODE,                    0x42)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_RECORD_CACHE_PLAY,              0x43)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_RECORD_CACHE_STREAM,            0x2D)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_CHECK_VERSION,                  0x49)        \
        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_SERVO,                        0x50)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_SERVO_MAX,                        0x51)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_SERVO_MIN,                        0x52)        \
        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_SYS_VOLUME,                     0x53)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_LOST_CTRL,                      0x54)        \
        X_CMD_MEDIA_MACRO(CZI_PROTOCOL_COMMON_CMD_STARTUP_MUTE,                     0x55)        \
        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_DTM_FREQ,                       0x62)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_LIGHT,                          0x63)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_BT_CTRL,                        0x64)        \
        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_AI_VIDEO,                       0x70)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_ACTIVATE,                       0x93)        \
        X_CMD_COMMON_MACRO(CZI_PROTOCOL_COMMON_CMD_LIGHTANDVOLUME,                       0x94)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_REMAIN_MEM,                      0x95)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_DEVICE_VOLUME,                      0x96)\
        \
        X_CMD_GPI_MACRO(CZI_PROTOCOL_COMMON_CMD_GPI,                      0x9A)         \
        \
        X_CMD_GL_MACRO(CZI_PROTOCOL_COMMON_CMD_GLIGHT,                              0xC1)        \
        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_PC_HANDSHAKE,                   0xF0)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_UPDATE_TRIGGER,                 0xF1)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_UPDATE_DATA,                    0xF2)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_LOG,                            0xF3)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_FILE_UPLOAD,                    0xF4)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_UPDATE_USER_FILE_TRIGGER,       0xF5)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_UPDATE_USER_FILE_DATA,          0xF6)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_GET_MEDIA_TEMPERATURE_INFO,     0xF7)        \
        X_CMD_FILE_MACRO(CZI_PROTOCOL_COMMON_CMD_PROTO_BUFF,                     0xFF)        \
        X_CMD_FILE_MACRO(ALARM_LIGHT_CMD_PARAM_SWITCH_LIGHT,                     0xD5)        \
        X_CMD_FILE_MACRO(ALARM_LIGHT_BREATHING_LIGHT,                            0xA3)        \


// X_CMD_COMMON_MACRO
// X_CMD_FILE_MACRO
// X_CMD_MEDIA_MACRO
// X_CMD_


typedef enum{
    // #define X_MACRO(a, b) a = b,
    #define X_CMD_COMMON_MACRO(a, b) a = b,
    #define X_CMD_FILE_MACRO(a, b) a = b,
    #define X_CMD_MEDIA_MACRO(a, b) a = b,
    #define X_CMD_GL_MACRO(a, b) a = b,
    #define X_CMD_GPI_MACRO(a, b) a = b,
    
    __PACKET_CMD

    // #undef X_MACRO
    #undef X_CMD_COMMON_MACRO
    #undef X_CMD_FILE_MACRO
    #undef X_CMD_MEDIA_MACRO
    #undef X_CMD_GL_MACRO
    #undef X_CMD_GPI_MACRO
}E_ProtocolCommandAll;

typedef enum{
    CMD_TYPE_COM = 1,
    CMD_TYPE_MEDIA,
    CMD_TYPE_FILE,
    CMD_TYPE_GL,
    CMD_TYPE_GPI,

}E_ProtocolCommandType;

#define PROT_MAX_LEN (1024 + 200) //1024 for ymodem, 200 for protocol and reserved
// #define PROT_LENGTH_EXCEPT 6
#define PROT_HEADER_DATA 0x24
#define PROT_TAILER_DATA 0x23
#define PROT_VALIDS_DATA 0x00

/* prot rsp data */
typedef struct
{
    unsigned char rspData[PROT_MAX_LEN];             /* rsp data */
    int rspLen;                                      /* rsp len */
    E_ProtocolCommandAll protCmd;                       /* prot command */
    unsigned char synced;                            /* sync data flag */

    /* TBD */
} T_ProtRspData;


typedef enum{
    CMD_INVALID = 0,
    CMD_UNKNOWN,
    CMD_VALID,
    CMD_VALID_BUT_DATA_MORE,
}E_checkCMD;

// int CziInternal_AntiStickedPackets(T_ProtCommonWorkSpace *work, unsigned char *data, int len, unsigned char *reply, int *replyLen);
// void Prot_ReceiverHandler(unsigned char *fullyData, unsigned short len, T_ProtRspData *rsp);
// int Prot_CommonHdl(E_Protocol_Type type, unsigned char *data, int len, T_ProtRspData *output);
int Prot_CommonPacketHandler(E_Protocol_Type type, E_ProtocolCommandAll protCmd, unsigned char *data, short int size, T_ProtRspData *rsp);
bool CziProt_CmdCheckValue(unsigned char cmd);
int CziProt_CheckCmdIsVaildType(int cmd);
unsigned char Prot_ValidateXOR(unsigned char *data, short int size);
#endif /* _PROTOCOL_COMMONHANDLER_H_ */
