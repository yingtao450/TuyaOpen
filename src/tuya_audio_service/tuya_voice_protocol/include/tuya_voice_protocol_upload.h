/**
 * @file tuya_voice_protocol_upload.h
 * @brief Defines the voice protocol upload interface for Tuya's audio service.
 *
 * This source file provides the implementation of voice data upload
 * functionality within the Tuya audio service framework. It includes the
 * initialization, configuration, and processing functions for audio upload,
 * which handles the transmission of voice data through multiple protocols
 * including MQTT, WebSocket, and HTTP. The implementation supports flexible
 * protocol selection and seamless switching between different transport
 * mechanisms. This file is essential for developers working on IoT applications
 * that require reliable voice data transmission and multi-protocol support.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TUYA_VOICE_PROTOCOL_UPLOAD_H__
#define __TUYA_VOICE_PROTOCOL_UPLOAD_H__

#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"

#include "tuya_voice_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TUYA_VOICE_UPLOAD_WS_VER_MAX_LEN     (16)

typedef enum {
    TUYA_VOICE_UPLOAD_TARGET_SPEECH,
    TUYA_VOICE_UPLOAD_TARGET_WECHAT,
    TUYA_VOICE_UPLOAD_TARGET_INVALD
} TUYA_VOICE_UPLOAD_TARGET_E;

typedef enum {
    TUYA_VOICE_UPLOAD_PROTOCOL_MQTT,
    TUYA_VOICE_UPLOAD_PROTOCOL_HTTP,
    TUYA_VOICE_UPLOAD_PROTOCOL_WS,
    TUYA_VOICE_UPLOAD_PROTOCOL_END
} TUYA_VOICE_UPLOAD_PROTOCOL_E;

typedef void * TUYA_VOICE_UPLOAD_T;

typedef struct {
    uint8_t  ver_id;                                          //< version id
    char  ver_string[TUYA_VOICE_UPLOAD_WS_VER_MAX_LEN];    //< version string
    uint8_t  mode;                                            //< encode quality mode
    uint8_t  mode_bit_stream_ver;                             //< encode quality mode bitstream version
    uint32_t  rate;                                            //< audio sample rate
    uint8_t  channels;                                        //< audio channels
    uint32_t  bit_rate;                                        //< encode bit rate
    uint32_t  frame_size;                                      //< encode frame size
    uint8_t  vbr;                                             //< encode vbr
    uint8_t  encode_frame_size;                               //< encode quality frame size
} TUYA_VOICE_WS_START_PARAMS_S;

OPERATE_RET tuya_voice_upload_start(TUYA_VOICE_UPLOAD_T *uploader, 
                                    TUYA_VOICE_AUDIO_FORMAT_E format,
                                    TUYA_VOICE_UPLOAD_TARGET_E target,
                                    char *p_session_id,
                                    uint8_t *p_params,
                                    uint32_t params_len);

OPERATE_RET tuya_voice_upload_send(TUYA_VOICE_UPLOAD_T uploader, uint8_t *buf, uint32_t len);

OPERATE_RET tuya_voice_upload_stop(TUYA_VOICE_UPLOAD_T uploader, BOOL_T force_stop);

OPERATE_RET tuya_voice_upload_get_message_id(TUYA_VOICE_UPLOAD_T uploader, char *buffer, int len);

#ifdef __cplusplus
}
#endif

#endif /* __TUYA_VOICE_PROTOCOL_UPLOAD_H__ */