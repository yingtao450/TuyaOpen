/**
 * @file tuya_voice_protocol_mqtt.h
 * @brief Defines the MQTT protocol interface for Tuya's voice service.
 *
 * This source file provides the implementation of MQTT protocol handling
 * functionality within the Tuya voice service framework. It includes the
 * initialization, configuration, and processing functions for voice data
 * transmission over MQTT, which handles the communication between IoT devices
 * and the cloud platform. The implementation supports various message types
 * and protocols to ensure reliable voice data exchange. This file is essential
 * for developers working on IoT applications that require voice control and
 * real-time audio communication capabilities through MQTT.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TUYA_IOT_VOICE_PROTOCOL_MQTT_H__
#define __TUYA_IOT_VOICE_PROTOCOL_MQTT_H__

#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_voice_protocol_upload.h"

#ifdef __cplusplus
extern "C" {
#endif

OPERATE_RET tuya_voice_proto_mqtt_init(TUYA_VOICE_CBS_S *cbs);

OPERATE_RET tuya_voice_proto_mqtt_deinit(void);

OPERATE_RET tuya_voice_proto_mqtt_audio_report_progress(uint32_t id, uint32_t offset, uint32_t total);

OPERATE_RET tuya_voice_proto_mqtt_audio_request_next(uint32_t id, BOOL_T need_tts);

OPERATE_RET tuya_voice_proto_mqtt_audio_request_prev(uint32_t id, BOOL_T need_tts);

OPERATE_RET tuya_voice_proto_mqtt_audio_request_current(void);

OPERATE_RET tuya_voice_proto_mqtt_audio_request_playmusic(void);

OPERATE_RET tuya_voice_proto_mqtt_audio_collect(uint32_t id);

OPERATE_RET tuya_voice_proto_mqtt_bell_request(char *bell_data_json);

OPERATE_RET tuya_voice_proto_mqtt_tts_complete_report(char *callback_val);

OPERATE_RET tuya_voice_proto_mqtt_tts_get(char *tts_content);

OPERATE_RET tuya_voice_proto_mqtt_devinfo_report(char *devinfo_json);

OPERATE_RET tuya_voice_proto_mqtt_common_report(char *p_data);

OPERATE_RET tuya_voice_proto_mqtt_thing_config_stop_report(void);

OPERATE_RET tuya_voice_proto_mqtt_thing_config_request_report(void);

OPERATE_RET tuya_voice_proto_mqtt_thing_config_reject_report(void);

OPERATE_RET tuya_voice_proto_mqtt_thing_config_access_count_report(int count);

OPERATE_RET tuya_voice_proto_mqtt_nick_name_report(TUYA_VOICE_NICK_NAME_OPRT_E mode, char *nickname, char *pinyin, BOOL_T set_result);

OPERATE_RET tuya_voice_proto_mqtt_dndmode_report(BOOL_T set_result, int stamp);

OPERATE_RET tuya_voice_proto_mqtt_dev_status_report(TUYA_VOICE_DEV_STATUS_E status);

OPERATE_RET tuya_voice_proto_mqtt_online_local_asr_sync(void);

OPERATE_RET tuya_voice_proto_mqtt_upload_start(TUYA_VOICE_UPLOAD_T *uploader,
                                    TUYA_VOICE_AUDIO_FORMAT_E format,
                                    TUYA_VOICE_UPLOAD_TARGET_E target,
                                    char *p_session_id,
                                    uint8_t *p_buf,
                                    uint32_t buf_len);

OPERATE_RET tuya_voice_proto_mqtt_upload_send(TUYA_VOICE_UPLOAD_T uploader, uint8_t *buf, uint32_t len);

OPERATE_RET tuya_voice_proto_mqtt_upload_stop(TUYA_VOICE_UPLOAD_T uploader, BOOL_T force_stop);

OPERATE_RET tuya_voice_proto_mqtt_upload_get_message_id(TUYA_VOICE_UPLOAD_T uploader, char *buffer, int len);

#ifdef __cplusplus
}
#endif

#endif /* __TUYA_IOT_VOICE_PROTOCOL_H__ */
