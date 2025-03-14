
/**
 * @file tuya_voice_protocol_upload.c
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

#include <stdio.h>
#include "tuya_cloud_types.h"
#include "tuya_voice_protocol_upload.h"

#include "tuya_voice_protocol_ws.h"
#include "tuya_voice_protocol_mqtt.h"

OPERATE_RET tuya_voice_upload_start(TUYA_VOICE_UPLOAD_T *uploader, 
                                    TUYA_VOICE_AUDIO_FORMAT_E format,
                                    TUYA_VOICE_UPLOAD_TARGET_E target,
                                    char *p_session_id,
                                    uint8_t *p_buf,
                                    uint32_t buf_len)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_upload_start(uploader, format, target, p_session_id, p_buf, buf_len);
    #endif

    #ifdef ENABLE_VOICE_PROTOCOL_HTTP
    return tuya_voice_proto_http_upload_start(uploader, format, target, p_session_id, p_buf, buf_len);
    #endif

    return tuya_voice_proto_mqtt_upload_start(uploader, format, target, p_session_id, p_buf, buf_len);
}

OPERATE_RET tuya_voice_upload_send(TUYA_VOICE_UPLOAD_T uploader, uint8_t *buf, uint32_t len)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_upload_send(uploader, buf, len);
    #endif

    #ifdef ENABLE_VOICE_PROTOCOL_HTTP
    return tuya_voice_proto_http_upload_send(uploader, buf, len);
    #endif

    return tuya_voice_proto_mqtt_upload_send(uploader, buf, len);
}

OPERATE_RET tuya_voice_upload_stop(TUYA_VOICE_UPLOAD_T uploader, BOOL_T force_stop)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_upload_stop(uploader, force_stop);
    #endif

    #ifdef ENABLE_VOICE_PROTOCOL_HTTP
    return tuya_voice_proto_http_upload_stop(uploader, force_stop);
    #endif

    return tuya_voice_proto_mqtt_upload_stop(uploader, force_stop);
}

OPERATE_RET tuya_voice_upload_get_message_id(TUYA_VOICE_UPLOAD_T uploader, char *buffer, int len)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_upload_get_message_id(uploader, buffer, len);
    #endif

    #ifdef ENABLE_VOICE_PROTOCOL_HTTP
    return tuya_voice_proto_http_upload_get_message_id(uploader, buffer, len);
    #endif

    return tuya_voice_proto_mqtt_upload_get_message_id(uploader, buffer, len);
}