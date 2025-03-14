/**
 * @file tuya_voice_json_parse.c
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

#include <stdio.h>
#include <string.h>
#include "tal_api.h"
#include "cJSON.h"
#include "tuya_voice_json_parse.h"
#include "tuya_voice_protocol.h"

#define PARSE_MEDIA_TYPE_TTS         (0)
#define PARSE_MEDIA_TYPE_AUDIO       (1)

char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *new_str = Malloc(len);
    if (new_str == NULL) {
        return NULL;
    }
    memcpy(new_str, s, len);
    return new_str;
}

static TUYA_VOICE_AUDIO_FORMAT_E __get_format(char *format)
{
    TUYA_VOICE_AUDIO_FORMAT_E fmt = TUYA_VOICE_AUDIO_FORMAT_MP3;

    if (strcmp(format, "mp3") == 0) {
        fmt = TUYA_VOICE_AUDIO_FORMAT_MP3;
    } else if (strcmp(format, "wav") == 0) {
        fmt = TUYA_VOICE_AUDIO_FORMAT_WAV;
    } else if (strcmp(format, "m4a") == 0) {
        fmt = TUYA_VOICE_AUDIO_FORMAT_M4A;
    } else if (strcmp(format, "aac") == 0) {
        fmt = TUYA_VOICE_AUDIO_FORMAT_AAC;
    } else if (strcmp(format, "amr") == 0) {
        fmt = TUYA_VOICE_AUDIO_FORMAT_AMR;
    } else if (strcmp(format, "flac") == 0) {
        fmt = TUYA_VOICE_AUDIO_FORMAT_FLAC;
    } else {
        PR_ERR("decode type invald:%s", format);
        fmt = TUYA_VOICE_AUDIO_FORMAT_INVALD;
    }

    return fmt;
}

static TUYA_VOICE_HTTP_METHOD_E __get_http_medhod(char *method)
{
    TUYA_VOICE_HTTP_METHOD_E mtd = TUYA_VOICE_HTTP_INVALD;

    if (strcmp(method, "post") == 0) {
        mtd = TUYA_VOICE_HTTP_POST;
    } else if (strcmp(method, "get") == 0) {
        mtd = TUYA_VOICE_HTTP_GET;
    }  else {
        PR_ERR("decode type invald:%s", method);
        mtd = TUYA_VOICE_HTTP_INVALD;
    }

    return mtd;
}

static TUYA_VOICE_TASK_TYPE_E __get_task_type(char *task_type)
{
    TUYA_VOICE_TASK_TYPE_E type = TUYA_VOICE_TASK_NORMAL;

    if (strcmp(task_type, "clock") == 0) {
        type = TUYA_VOICE_TASK_CLOCK;
    } else if (strcmp(task_type, "alert") == 0) {
        type = TUYA_VOICE_TASK_ALERT;
    } else if (strcmp(task_type, "bell") == 0) {
        type = TUYA_VOICE_TASK_RING_TONE;
    } else if (strcmp(task_type, "call") == 0) {
        type = TUYA_VOICE_TASK_CALL;
    } else if (strcmp(task_type, "call_tts") == 0) {
        type = TUYA_VOICE_TASK_CALL_TTS;
    }

    return type;
}

static OPERATE_RET __parse_voice_media_tts(cJSON *json, int media_type, TUYA_VOICE_TTS_S **tts)
{
    cJSON *req_type = NULL, *tts_url = NULL, *req_body = NULL, *format = NULL;

    if (media_type == PARSE_MEDIA_TYPE_TTS) {
        req_type = cJSON_GetObjectItem(json, "httpRequestType");
        tts_url = cJSON_GetObjectItem(json, "ttsUrl");
        format = cJSON_GetObjectItem(json, "format");
        req_body = cJSON_GetObjectItem(json, "requestBody");
    } else if (media_type == PARSE_MEDIA_TYPE_AUDIO) {
        req_type = cJSON_GetObjectItem(json, "preRequestType");
        tts_url = cJSON_GetObjectItem(json, "preTtsUrl");
        format = cJSON_GetObjectItem(json, "preFormat");
        req_body = cJSON_GetObjectItem(json, "preRequestBody");
        if (tts_url == NULL || strlen(tts_url->valuestring) == 0) {
            PR_DEBUG("pre tts is not exist");
            *tts = NULL;
            return OPRT_OK;
        }
    } else {
        return OPRT_CJSON_GET_ERR;
    }

    cJSON *keep_session = cJSON_GetObjectItem(json, "keepSession");
    cJSON *session_id = cJSON_GetObjectItem(json, "sessionId");
    cJSON *callback_val = cJSON_GetObjectItem(json, "callbackValue");
    cJSON *message_id = cJSON_GetObjectItem(json, "messageId");
    cJSON *task_type = cJSON_GetObjectItem(json, "taskType");

    if (keep_session == NULL) {
        PR_ERR("input is invalid");
        return OPRT_CJSON_GET_ERR;
    }

    TUYA_VOICE_TTS_S *p_tts = Malloc(sizeof(TUYA_VOICE_TTS_S));
    if (p_tts == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    memset(p_tts, 0, sizeof(TUYA_VOICE_TTS_S));

    p_tts->keep_session = FALSE;
    if (keep_session->type == cJSON_True) {
        p_tts->keep_session = TRUE;
    }

    if (task_type == NULL) {
        p_tts->task_type = TUYA_VOICE_TASK_NORMAL;
    } else {
        p_tts->task_type = __get_task_type(task_type->valuestring);
    }

    if (format == NULL) {
        p_tts->format = TUYA_VOICE_AUDIO_FORMAT_MP3;
    } else {
        p_tts->format = __get_format(format->valuestring);
    }

    if (p_tts->format == TUYA_VOICE_AUDIO_FORMAT_INVALD) {
        Free(p_tts);
        return OPRT_CJSON_GET_ERR;
    }

    if (NULL != tts_url) {
        if ((p_tts->url = my_strdup(tts_url->valuestring)) == NULL) {
            PR_ERR("malloc fail. url:%s", tts_url->valuestring);
            Free(p_tts);
            return OPRT_MALLOC_FAILED;
        }
    }

    if (NULL != req_type) {
        if ((p_tts->http_method = __get_http_medhod(req_type->valuestring)) == TUYA_VOICE_HTTP_INVALD) {
            PR_ERR("req_type %s is invalid ", req_type->valuestring);
            Free(p_tts->url);
            Free(p_tts);
            return OPRT_CJSON_GET_ERR;
        }
    }

    if (p_tts->http_method == TUYA_VOICE_HTTP_POST && req_body != NULL) {
        if ((p_tts->req_body = my_strdup(req_body->valuestring)) == NULL) {
            PR_ERR("malloc fail. body:%s",  req_body->valuestring);
            Free(p_tts->url);
            Free(p_tts);
            return OPRT_MALLOC_FAILED;
        }
    }

    if (session_id) {
        int session_id_len = strlen(session_id->valuestring);
        if (session_id_len > TUYA_VOICE_SESSION_ID_MAX_LEN || session_id_len <= 0) {
            PR_ERR("session_id_len %d error", session_id_len);
            Free(p_tts->url);
            Free(p_tts->req_body);
            Free(p_tts);
            return OPRT_CJSON_GET_ERR;
        }

        strncpy(p_tts->session_id, session_id->valuestring, sizeof(p_tts->session_id));
    }

    if (message_id) {
        int message_id_len = strlen(message_id->valuestring);
        if (message_id_len > TUYA_VOICE_MESSAGE_ID_MAX_LEN || message_id_len <= 0) {
            PR_ERR("message_id_len %d error", message_id_len);
            Free(p_tts->url);
            Free(p_tts->req_body);
            Free(p_tts);
            return OPRT_CJSON_GET_ERR;
        }
        strncpy(p_tts->message_id, message_id->valuestring, message_id_len);
    }

    if (callback_val && callback_val->type == cJSON_String) {
        int callback_val_len = strlen(callback_val->valuestring);
        if (callback_val_len > TUYA_VOICE_CALLBACK_VAL_MAX_LEN || callback_val_len <= 0) {
            PR_ERR("callback_val_len %d error", callback_val_len);
            Free(p_tts->url);
            Free(p_tts->req_body);
            Free(p_tts);
            return OPRT_CJSON_GET_ERR;
        }

        strcpy(p_tts->callback_val, callback_val->valuestring);
    }

    *tts = p_tts;

    return OPRT_OK;
}

static OPERATE_RET __parse_voice_play_audio_item(cJSON *json, TUYA_VOICE_MEDIA_SRC_S *p_media_src)
{
    cJSON *id = cJSON_GetObjectItem(json, "id");
    cJSON *url = cJSON_GetObjectItem(json, "url");
    cJSON *req_body = cJSON_GetObjectItem(json, "requestBody");
    cJSON *req_type = cJSON_GetObjectItem(json, "requestType");
    cJSON *format = cJSON_GetObjectItem(json, "format");
    cJSON *duration = cJSON_GetObjectItem(json, "duration");
    cJSON *size = cJSON_GetObjectItem(json, "size");
    cJSON *songName = cJSON_GetObjectItem(json, "songName");
    cJSON *artist = cJSON_GetObjectItem(json, "artist");


    if ((id == NULL) || (format == NULL) || (req_type == NULL) || (url == NULL)) {
        PR_ERR("input is invalid");
        return OPRT_CJSON_GET_ERR;
    }

    p_media_src->id = id->valueint;

    p_media_src->format = __get_format(format->valuestring);
    if (p_media_src->format == TUYA_VOICE_AUDIO_FORMAT_INVALD) {
        PR_ERR("decode type invald:%s", format->valuestring);
        return OPRT_CJSON_GET_ERR;
    }

    if (duration != NULL) {
        p_media_src->duration = duration->valueint;
    }

    if (size != NULL) {
        p_media_src->length = size->valueint;
    }

    if (songName != NULL) {
        snprintf(p_media_src->song_name, sizeof(p_media_src->song_name), "%s", songName->valuestring);
    }

    if (artist != NULL) {
        snprintf(p_media_src->artist, sizeof(p_media_src->artist), "%s", artist->valuestring);
    }

    if ((p_media_src->url = my_strdup(url->valuestring)) == NULL) {
        PR_ERR("malloc fail. url:%s", url->valuestring);
        return OPRT_MALLOC_FAILED;
    }

    if ((p_media_src->http_method = __get_http_medhod(req_type->valuestring)) == TUYA_VOICE_HTTP_INVALD) {
        PR_ERR("req_type %s is invalid ", req_type->valuestring);
        return OPRT_CJSON_GET_ERR;
    }

    if (p_media_src->http_method == TUYA_VOICE_HTTP_POST && req_body != NULL) {
        if ((p_media_src->req_body = my_strdup(req_body->valuestring)) == NULL) {
            PR_ERR("malloc fail. body:%s",  req_body->valuestring);
            return OPRT_MALLOC_FAILED;
        }
    }

    return OPRT_OK;
}

OPERATE_RET tuya_voice_json_parse_tts(cJSON *json, TUYA_VOICE_TTS_S **tts)
{
    OPERATE_RET      ret = OPRT_OK;
    TUYA_VOICE_TTS_S *p_tts = NULL;

    if (json == NULL || tts == NULL) {
        return OPRT_INVALID_PARM;
    }

    if ((ret = __parse_voice_media_tts(json, PARSE_MEDIA_TYPE_TTS, &p_tts)) != OPRT_OK) {
        PR_ERR("__parse_voice_media_tts error:%d", ret);
        return ret;
    }

    *tts = p_tts;

    return OPRT_OK;
}

void tuya_voice_json_parse_free_tts(TUYA_VOICE_TTS_S *tts)
{
    if (tts != NULL) {
        Free(tts->url);
        Free(tts->req_body);
        Free(tts);
    }
}

OPERATE_RET tuya_voice_json_parse_media(cJSON *json, TUYA_VOICE_MEDIA_S **media)
{
    OPERATE_RET             ret = OPRT_OK;
    cJSON               *p_audiojson = NULL;
    TUYA_VOICE_TTS_S       *p_pre_tts = NULL;
    TUYA_VOICE_MEDIA_SRC_S *media_src = NULL;

    cJSON *audioUrlList = cJSON_GetObjectItem(json, "audioList");
    if (NULL == audioUrlList) {
        PR_ERR("audioUrlList is NULL");
        return OPRT_CJSON_GET_ERR;
    }

    int audio_num = cJSON_GetArraySize(audioUrlList);
    if (audio_num == 0) {
        PR_ERR("audio url list is empty");
    }

    if ((ret = __parse_voice_media_tts(json, PARSE_MEDIA_TYPE_AUDIO, &p_pre_tts)) != OPRT_OK) {
        PR_ERR("__parse_voice_media_tts error:%d", ret);
        return ret;
    }

    TUYA_VOICE_MEDIA_S *p_media = Malloc(sizeof(TUYA_VOICE_MEDIA_S));
    if (p_media == NULL) {
        PR_ERR("malloc arr fail.");
        tuya_voice_json_parse_free_tts(p_pre_tts);
        return OPRT_MALLOC_FAILED;
    }

    memset(p_media, 0, sizeof(sizeof(TUYA_VOICE_MEDIA_S)));

    p_media->pre_tts = p_pre_tts;
    p_media->src_cnt = audio_num;
    *media = p_media;

    if (p_media->src_cnt == 0) {
        return OPRT_OK;
    }

    p_media->src_array = Malloc(sizeof(TUYA_VOICE_MEDIA_SRC_S) * audio_num);
    if (p_media->src_array == NULL) {
        PR_ERR("malloc arr fail.");
        tuya_voice_json_parse_free_media(p_media);
        return OPRT_MALLOC_FAILED;
    }

    memset(p_media->src_array, 0, sizeof(TUYA_VOICE_MEDIA_SRC_S) * audio_num);

    int index = 0;
    for (index = 0; index < p_media->src_cnt; index++) {
        media_src = &p_media->src_array[index];
        p_audiojson = cJSON_GetArrayItem(audioUrlList, index);

        if (__parse_voice_play_audio_item(p_audiojson, media_src) != OPRT_OK) {
            PR_ERR("parse audio %d fail.", index);
            tuya_voice_json_parse_free_media(p_media);
            return OPRT_CJSON_PARSE_ERR;
        }
    }

    return OPRT_OK;
}

void tuya_voice_json_parse_free_media(TUYA_VOICE_MEDIA_S *p_media)
{
    if (p_media == NULL) {
        return ;
    }

    int i = 0;
    TUYA_VOICE_MEDIA_SRC_S *media_src = NULL;

    tuya_voice_json_parse_free_tts(p_media->pre_tts);
    for (i = 0; i < p_media->src_cnt; i++) {
        media_src = &p_media->src_array[i];
        Free(media_src->url);
        Free(media_src->req_body);
    }

    if (p_media->src_cnt > 0) {
        Free(p_media->src_array);
    }

    Free(p_media);
}

OPERATE_RET tuya_voice_json_parse_call_info(cJSON *json, TUYA_VOICE_CALL_PHONE_INFO_S **call_info)
{
    OPERATE_RET             ret = OPRT_OK;
    TUYA_VOICE_TTS_S       *p_tts = NULL;

    cJSON *data = NULL, *resourceId = NULL, *resourceName = NULL, *resourceType = NULL;

    if (json == NULL || call_info == NULL) {
        return OPRT_INVALID_PARM;
    }

    data = cJSON_GetObjectItem(json, "data");
    if (data == NULL) {
        PR_ERR("parse call info error");
        return OPRT_CJSON_PARSE_ERR;
    }
    resourceId = cJSON_GetObjectItem(data, "resourceId");
    resourceName = cJSON_GetObjectItem(data, "resourceName");
    resourceType = cJSON_GetObjectItem(data, "resourceType");

    if (resourceId == NULL || resourceName == NULL || resourceType == NULL) {
        PR_ERR("parse call info error");
        return OPRT_CJSON_PARSE_ERR;
    }

    if ((ret = __parse_voice_media_tts(json, PARSE_MEDIA_TYPE_TTS, &p_tts)) != OPRT_OK) {
        PR_ERR("__parse_voice_media_tts error:%d", ret);
        return ret;
    }

    if (p_tts != NULL) {
        p_tts->task_type = TUYA_VOICE_TASK_CALL;
    }

    TUYA_VOICE_CALL_PHONE_INFO_S *p_call_info = Malloc(sizeof(TUYA_VOICE_CALL_PHONE_INFO_S));
    if (p_call_info == NULL) {
        PR_ERR("malloc call info fail.");
        tuya_voice_json_parse_free_tts(p_tts);
        return OPRT_MALLOC_FAILED;
    }

    memset(p_call_info, 0, sizeof(sizeof(TUYA_VOICE_CALL_PHONE_INFO_S)));

    p_call_info->pre_tts = p_tts;
    snprintf(p_call_info->target_id, sizeof(p_call_info->target_id), "%s", resourceId->valuestring);
    snprintf(p_call_info->target_name, sizeof(p_call_info->target_name), "%s", resourceName->valuestring);
    p_call_info->type = resourceType->valueint;

    *call_info = p_call_info;

    return OPRT_OK;
}

void tuya_voice_json_parse_free_call_info(TUYA_VOICE_CALL_PHONE_INFO_S *call_info)
{
    if (call_info) {
        tuya_voice_json_parse_free_tts(call_info->pre_tts);
        Free(call_info);
    }
}


