/**
 * @file tuya_voice_protocol.c
 * @brief Defines the core voice protocol interface for Tuya's audio service.
 *
 * This source file provides the implementation of core voice protocol
 * functionality within the Tuya audio service framework. It includes the
 * initialization, configuration, and processing functions for voice protocols,
 * which handles the integration and management of multiple transport protocols
 * including MQTT and WebSocket. The implementation supports various voice
 * features such as TTS, audio streaming, device status reporting, and protocol
 * switching capabilities. This file is essential for developers working on IoT
 * applications that require flexible voice communication protocols and
 * comprehensive audio service management.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#include <stdio.h>
#include <string.h>

#include "tal_api.h"
#include "cJSON.h"

#include "tuya_voice_protocol_mqtt.h"
#include "tuya_voice_protocol_ws.h"

#include "tuya_voice_protocol.h"
#include "tuya_voice_protocol_mqtt.h"

OPERATE_RET tuya_voice_proto_init(TUYA_VOICE_CBS_S *cbs)
{
    OPERATE_RET rt = OPRT_OK;
    rt = tuya_voice_proto_mqtt_init(cbs);
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    rt |= tuya_voice_proto_ws_init(cbs);
    #endif

    return rt;
}

OPERATE_RET tuya_voice_proto_deinit(void)
{
    OPERATE_RET rt = OPRT_OK;

    rt = tuya_voice_proto_mqtt_deinit();
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    rt |= tuya_voice_proto_ws_deinit();
    #endif

    return  rt;
}

OPERATE_RET tuya_voice_proto_audio_report_progress(uint32_t id, uint32_t offset, uint32_t total)
{
    return tuya_voice_proto_mqtt_audio_report_progress(id, offset, total);
}

OPERATE_RET tuya_voice_proto_audio_request_next(uint32_t id, BOOL_T need_tts)
{
    return tuya_voice_proto_mqtt_audio_request_next(id, need_tts);
}

OPERATE_RET tuya_voice_proto_audio_request_prev(uint32_t id, BOOL_T need_tts)
{
    return tuya_voice_proto_mqtt_audio_request_prev(id, need_tts);
}

OPERATE_RET tuya_voice_proto_audio_request_current(void)
{
    return tuya_voice_proto_mqtt_audio_request_current();
}

OPERATE_RET tuya_voice_proto_audio_request_playmusic(void)
{
    return tuya_voice_proto_mqtt_audio_request_playmusic();
}

OPERATE_RET tuya_voice_proto_audio_collect(uint32_t id)
{
    return tuya_voice_proto_mqtt_audio_collect(id);
}

OPERATE_RET tuya_voice_proto_bell_request(char *bell_data_json)
{
    return tuya_voice_proto_mqtt_bell_request(bell_data_json);
}

OPERATE_RET tuya_voice_proto_tts_complete_report(char *callback_val)
{
    return tuya_voice_proto_mqtt_tts_complete_report(callback_val);
}

OPERATE_RET tuya_voice_proto_tts_get(char *tts_content)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_get_tts_stream(tts_content);
    #else
    return tuya_voice_proto_mqtt_tts_get(tts_content);
    #endif
}

OPERATE_RET tuya_voice_proto_devinfo_report(char *devinfo_json)
{
    return tuya_voice_proto_mqtt_devinfo_report(devinfo_json);
}

OPERATE_RET tuya_voice_proto_common_report(char *p_data)
{
    return tuya_voice_proto_mqtt_common_report(p_data);
}

OPERATE_RET tuya_voice_proto_thing_config_stop_report(void)
{
    return tuya_voice_proto_mqtt_thing_config_stop_report();
}

OPERATE_RET tuya_voice_proto_thing_config_request_report(void)
{
    return tuya_voice_proto_mqtt_thing_config_request_report();
}

OPERATE_RET tuya_voice_proto_thing_config_reject_report(void)
{
    return tuya_voice_proto_mqtt_thing_config_reject_report();
}

OPERATE_RET tuya_voice_proto_thing_config_access_count_report(int count)
{
    return tuya_voice_proto_mqtt_thing_config_access_count_report(count);
}

OPERATE_RET tuya_voice_proto_nick_name_report(TUYA_VOICE_NICK_NAME_OPRT_E oprt, char *nickname, char *pinyin, BOOL_T set_result)
{
    return tuya_voice_proto_mqtt_nick_name_report(oprt, nickname, pinyin, set_result);
}

OPERATE_RET tuya_voice_proto_dndmode_report(BOOL_T set_result, int stamp)
{
    return tuya_voice_proto_mqtt_dndmode_report(set_result, stamp);
}

OPERATE_RET tuya_voice_proto_dev_status_report(TUYA_VOICE_DEV_STATUS_E status)
{
    return tuya_voice_proto_mqtt_dev_status_report(status);
}

OPERATE_RET tuya_voice_proto_online_local_asr_sync(void)
{
    return tuya_voice_proto_mqtt_online_local_asr_sync();
}

OPERATE_RET tuya_voice_proto_start(void)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_client_start();
    #else
    return OPRT_NOT_SUPPORTED;
    #endif
}

OPERATE_RET tuya_voice_proto_stop(void)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_client_stop();
    #else
    return OPRT_NOT_SUPPORTED;
    #endif
}

OPERATE_RET tuya_voice_proto_del_domain_name(void)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_del_domain_name();
    #else
    return OPRT_NOT_SUPPORTED;
    #endif
}

OPERATE_RET tuya_voice_proto_get_tts_text(char *p_tts_text)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_get_tts_text(p_tts_text);
    #else
    return OPRT_NOT_SUPPORTED;
    #endif
}

OPERATE_RET tuya_voice_proto_get_tts_audio(char *p_session_id, char *p_tts_text, char *p_declaimer)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_get_tts_audio(p_session_id, p_tts_text, p_declaimer);
    #else
    return OPRT_NOT_SUPPORTED;
    #endif
}

BOOL_T tuya_voice_proto_is_online(void)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_is_online();
    #else
    return TRUE;
    #endif
}

void tuya_voice_proto_disconnect(void)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    tuya_voice_proto_ws_disconnect();
    #else
    return;
    #endif
}
OPERATE_RET tuya_voice_proto_interrupt(void)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    return tuya_voice_proto_ws_interrupt();
    #else
    return OPRT_NOT_SUPPORTED;
    #endif
}
void tuya_voice_proto_set_keepalve(uint32_t sec)
{
    #ifdef ENABLE_VOICE_PROTOCOL_STREAM_GW
    tuya_voice_proto_ws_set_keepalive(sec);
    #else
    return;
    #endif
}