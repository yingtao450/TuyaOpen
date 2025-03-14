/**
 * @file tuya_voice_json_parse.h
 * @brief Defines the voice protocol JSON parsing interface for Tuya's audio service.
 *
 * This source file provides the implementation of JSON parsing functionality
 * within the Tuya voice protocol framework. It includes the parsing functions
 * for various voice-related data structures, including TTS configurations,
 * media sources, and call information. The implementation supports comprehensive
 * JSON parsing for voice commands, audio formats, and communication parameters.
 * This file is essential for developers working on IoT applications that require
 * voice control and audio processing capabilities through JSON-based protocols.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TUYA_IOT_VOICE_JSON_PARSE_H__
#define __TUYA_IOT_VOICE_JSON_PARSE_H__

#include "tuya_voice_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

OPERATE_RET tuya_voice_json_parse_tts(cJSON *json, TUYA_VOICE_TTS_S **tts);

void tuya_voice_json_parse_free_tts(TUYA_VOICE_TTS_S *tts);

OPERATE_RET tuya_voice_json_parse_media(cJSON *json, TUYA_VOICE_MEDIA_S **media);

void tuya_voice_json_parse_free_media(TUYA_VOICE_MEDIA_S *p_media);

OPERATE_RET tuya_voice_json_parse_call_info(cJSON *json, TUYA_VOICE_CALL_PHONE_INFO_S **call_info);

void tuya_voice_json_parse_free_call_info(TUYA_VOICE_CALL_PHONE_INFO_S *call_info);

#ifdef __cplusplus
}
#endif

#endif /* __TUYA_IOT_VOICE_JSON_PARSE_H__ */


