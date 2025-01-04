#ifndef _PSDK_CONFIG_H_
#define _PSDK_CONFIG_H_

#define FILEPATH_MUSIC_LIST           "/czzn/cusr/mediaFileListInfoOfMusic"
#define FILEPATH_DJIRECORD_LIST       "/czzn/cusr/mediaFileListInfoOfDjiRecord"

#define FILEPATH_MUSIC_FILE "/czzn/cusr/music"
#define FILEPATH_DJIRECORD_FILE   "/czzn/cusr/dji_record"

#define FILEPATH_JSON_CONFIG_USR  "/czzn/cusr/config_usr.json"
#define JSON_KEY_STARTUP_MUTE     "startup_mute" 
#define JSON_KEY_BREATH_LIGHT     "breath_light" 

#define FILEPATH_JSON_STATE_MEDIA "/czzn/carch/state/state_media.json"
#define JSON_KEY_MASTER_VOLUME    "master_volume" 
#define JSON_KEY_LOOP             "loop"

#define FILEPATH_JSON_STATE_TTS "/czzn/carch/state/state_tts.json"
#define JSON_KEY_TTS_VOICE        "tts_voice" 
#define JSON_KEY_TTS_SPEED        "tts_speed"

#define MAXLEN_FILE_NAME    (128)
#define MAXLEN_FILEPATH     (256)

#define STORAGE_PATH                   "/"


#define STACK_SIZE_CZI_PILOT_HANDLER       (1024 * 10)
#define MAXLEN_SEND_TO_MASK                (127)
#define MAXNUM_TIMEOUT_COUNT               (3)
#define BUFFER_SIZE                        50

#define STACK_SIZE_PSDK_TO_PROT_HANDLER       (1024 * 10)
#define STACK_SIZE_PROT_TO_PSDK_HANDLER       (1024 * 10)

#define DETECT_STATE_FILE_PATH "/tmp/detect_state"
#define FILEPATH_AI_DETECT_STATE "/czzn/carch/state/state_aidetect.json"
#define FILEPATH_MODEL_CONFIG_JSON "/czzn/cmodel/model_config.json"
#define JSON_KEY_AI_DETECT_RESULT "detect_result"

#define FILEPATH_JSON_STATE_PSDK       "/czzn/carch/state/state_psdk.json"
#define JSON_KEY_AIRCRAFT_STATUS       "aircraft_state"
#define JSON_KEY_ANGLE_TUNING          "angle_tuning"

#define VIDEO_STREAM_STATE_FILE_PATH "/tmp/state_video_stream"
#define SHARED_MEMORY_SIZE (10 * 1024 * 1024)  // 10 MB

#define FILEPATH_CONFIG_UPDATE_JSON  "/czzn/carch/config/config_update.json"
#define JSON_KEY_UPDATE              "update"

#define WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_FOR_REAL_RECORD   (16000)
#define WIDGET_SPEAKER_AUDIO_OPUS_DECODE_BITRATE_FOR_MUSIC         (32000)

#define DIR_TYPE_MUSIC    (0x00)

#define TOTAL_CURRENT (3.25)
#define HIGHPOWER_LIMIT (51)
#define LOWPOWER_LIMIT  (43)

#endif