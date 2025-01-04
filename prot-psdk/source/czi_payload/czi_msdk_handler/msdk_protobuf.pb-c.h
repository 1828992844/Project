/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: msdk_protobuf.proto */

#ifndef PROTOBUF_C_msdk_5fprotobuf_2eproto__INCLUDED
#define PROTOBUF_C_msdk_5fprotobuf_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1005000 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct CziProtobufInfo CziProtobufInfo;


/* --- enums --- */


/* --- messages --- */

struct  CziProtobufInfo
{
  ProtobufCMessage base;
  char *activation_timestamp;
  char *model;
  char *sn;
  char *software_version;
  char *hardware_version;
  /*
   * 警灯爆闪灯码；0x00关，0x01红蓝，0x02红，0x03蓝，0x04黄，0x05绿
   */
  int32_t light_code;
  /*
   * 云台角度 0-90
   */
  int32_t gimbal_angle;
  /*
   * 当前主通道音量 0-100
   */
  int32_t master_volume;
  /*
   * 当前背景音音量 0-100
   */
  int32_t background_volume;
  /*
   * 喊话器开关 播放状态码
   */
  int32_t megaphone_switch;
  /*
   * 喊话器模式 0x10 tts; 0x21 音乐播放模式; 0x29实时喊话
   */
  int32_t megaphone_model;
  /*
   * 播放循环模式 循环状态码
   */
  int32_t loop_model;
  /*
   * 监听状态 播放状态码
   */
  int32_t islistening;
  /*
   * F1 开启 F0 关闭
   */
  int32_t startup_mute;
  /*
   * F1 继续 F0 停止
   */
  int32_t loss_action;
  /*
   * 1 开 0 关
   */
  int32_t breath_light;
  /*
   * 
   */
  int32_t tts_voice;
  /*
   * 
   */
  int32_t tts_speed;
};
#define CZI_PROTOBUF_INFO__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&czi_protobuf_info__descriptor) \
    , (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }


/* CziProtobufInfo methods */
void   czi_protobuf_info__init
                     (CziProtobufInfo         *message);
size_t czi_protobuf_info__get_packed_size
                     (const CziProtobufInfo   *message);
size_t czi_protobuf_info__pack
                     (const CziProtobufInfo   *message,
                      uint8_t             *out);
size_t czi_protobuf_info__pack_to_buffer
                     (const CziProtobufInfo   *message,
                      ProtobufCBuffer     *buffer);
CziProtobufInfo *
       czi_protobuf_info__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   czi_protobuf_info__free_unpacked
                     (CziProtobufInfo *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*CziProtobufInfo_Closure)
                 (const CziProtobufInfo *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor czi_protobuf_info__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_msdk_5fprotobuf_2eproto__INCLUDED */
