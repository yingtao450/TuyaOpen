
/**
 * @file tuya_voice_protocol_mqtt.c
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

#include <stdio.h>
#include <string.h>


#include "tal_api.h"
#include "cJSON.h"
#include "tuya_voice_json_parse.h"
#include "tuya_voice_protocol_mqtt.h"
#include "tuya_voice_protocol.h"
#include "tuya_iot.h"
#include "mqtt_service.h"

#define VOICE_MQ_PROTOCOL_NUM           (501)
#define TUYA_SPEAKER_MQTT_REPORT_MAX    (4 * 1024)
#define MQ_UPLOAD_PUB_TOPIC             "v/m/o/"
#define MQ_UPLOAD_PUB_TOPIC_LEN         sizeof(MQ_UPLOAD_PUB_TOPIC)
#define SPEAKER_UPLOAD_ID_MIN           (1)
#define SPEAKER_UPLOAD_ID_MAX           (0x7ffffffa) /**< FIXME: 0xfffffffa what? why? */

typedef enum {
    TY_UPLOAD_START = 0,
    TY_UPLOAD_MID = 1,
    TY_UPLOAD_END = 2
} TY_MEDIA_UPLOAD_FLAG_E;

typedef struct {
    OPERATE_RET            op_ret;
    SEM_HANDLE             sem_handle;
} SYNC_REPORT;

typedef struct
{
    uint8_t               version;
    uint32_t               pack_num;
    uint8_t               upload_flag;        //TY_MEDIA_UPLOAD_FLAG_E
    uint32_t               req_id;
    uint8_t               voice_encode;       //TY_MEDIA_ENCODE_T
    uint8_t               target;             //TY_MEDIA_TARGET_T
    char               session_id[TUYA_VOICE_MESSAGE_ID_MAX_LEN];
    uint32_t               data_len;
    unsigned long long   message_id;
    uint8_t               data[0];
} PACKED TY_VOICE_MQTT_UPLOAD_DATA_S;

typedef struct
{
    uint32_t                       data_len;
    TY_VOICE_MQTT_UPLOAD_DATA_S  upload_head;
    char                       upload_send_topic[MQ_UPLOAD_PUB_TOPIC_LEN + GW_ID_LEN];
} TY_VOICE_MQTT_UPLOAD_CTX_S;

static TUYA_VOICE_CBS_S g_voice_mqtt_cbs = {0};
static uint32_t tuya_speaker_get_req_id();
static OPERATE_RET __pack_upload_data(TY_VOICE_MQTT_UPLOAD_DATA_S *p_in, TY_VOICE_MQTT_UPLOAD_DATA_S *p_out);
// static OPERATE_RET __voice_mqc_proto_cb(cJSON *root_json);
static OPERATE_RET __send_custom_mqtt_msg(char *p_data);
static OPERATE_RET __send_custom_mqtt_msg_wait(char *p_data, int overtime_s);
static void __voice_mqc_proto_cb(tuya_protocol_event_t* ev);


OPERATE_RET tuya_voice_proto_mqtt_init(TUYA_VOICE_CBS_S *cbs)
{
    if (NULL == cbs) {
        PR_ERR("invalid parm");
        return OPRT_INVALID_PARM;
    }

    tuya_iot_client_t *iot_client = tuya_iot_client_get();
    tuya_mqtt_protocol_register(&iot_client->mqctx, VOICE_MQ_PROTOCOL_NUM, __voice_mqc_proto_cb, NULL);

    memcpy(&g_voice_mqtt_cbs, cbs, sizeof(TUYA_VOICE_CBS_S));

    PR_DEBUG("mqc_app_register_cb %d ok", VOICE_MQ_PROTOCOL_NUM);

    return OPRT_OK;
}

OPERATE_RET tuya_voice_proto_mqtt_deinit(void)
{
    return 0;
}

OPERATE_RET tuya_voice_proto_mqtt_audio_report_progress(uint32_t id, uint32_t offset, uint32_t total)
{
    char data[256] = {0};

    snprintf(data, 256, "{\"type\":\"syncAudioProgress\",\"data\":{\"id\":%u,\"totalTime\":%u,\"timeOffset\":%u}}", id, total, offset);

    return __send_custom_mqtt_msg(data);
}

OPERATE_RET tuya_voice_proto_mqtt_audio_request_next(uint32_t id, BOOL_T need_tts)
{
    char data[256] = {0};
    if (need_tts) {
        snprintf(data, 256, "{\"type\":\"%s\",\"data\":{\"id\":%u}}", "next", id);
    } else {
        snprintf(data, 256, "{\"type\":\"%s\",\"data\":{\"id\":%u, \"preTtsFlag\":false}}", "next", id);
    }

    return __send_custom_mqtt_msg_wait(data, 2);
}

OPERATE_RET tuya_voice_proto_mqtt_audio_request_prev(uint32_t id, BOOL_T need_tts)
{
    char data[256] = {0};

    if (need_tts) {
        snprintf(data, 256, "{\"type\":\"%s\",\"data\":{\"id\":%u}}", "prev", id);
    } else {
        snprintf(data, 256, "{\"type\":\"%s\",\"data\":{\"id\":%u, \"preTtsFlag\":false}}", "prev", id);
    }
    return __send_custom_mqtt_msg_wait(data, 2);
}

OPERATE_RET tuya_voice_proto_mqtt_audio_request_current(void)
{
    char data[256] = {0};

    snprintf(data, 256, "{\"type\":\"current\",\"data\":{\"id\":%u}}", 0);
    return __send_custom_mqtt_msg_wait(data, 2);
}

OPERATE_RET tuya_voice_proto_mqtt_audio_request_playmusic(void)
{
    char data[256] = {0};

    snprintf(data, 256, "{\"type\":\"playMusic\",\"data\":{\"id\":%u}}", 0);
    return __send_custom_mqtt_msg_wait(data, 2);
}

OPERATE_RET tuya_voice_proto_mqtt_audio_collect(uint32_t id)
{
    char data[128] = {0};

    snprintf(data, 128, "{\"type\":\"collectAudio\",\"data\":{\"id\":%u}}", id);
    return __send_custom_mqtt_msg_wait(data, 2);
}

OPERATE_RET tuya_voice_proto_mqtt_bell_request(char *bell_data_json)
{
    if (NULL == bell_data_json) {
        return OPRT_INVALID_PARM;
    }

    char *p_data = NULL;
    int  len = strlen(bell_data_json) + 128;

    if ((p_data = Malloc(len)) == NULL) {
        PR_ERR("Malloc failed");
        return OPRT_MALLOC_FAILED;
    }

    memset(p_data, 0, len);
    snprintf(p_data, len, "{\"type\":\"requestBell\",\"data\":%s}", bell_data_json);
    OPERATE_RET ret = __send_custom_mqtt_msg(p_data);
    Free(p_data);

    return ret;
}

OPERATE_RET tuya_voice_proto_mqtt_tts_complete_report(char *callback_val)
{
    if (NULL == callback_val) {
        return OPRT_INVALID_PARM;
    }

    char data[256] = {0};

    snprintf(data, 256, "{\"type\":\"completeTts\",\"data\":{\"callbackValue\":\"%s\"}}", callback_val);
    return __send_custom_mqtt_msg(data);
}

OPERATE_RET tuya_voice_proto_mqtt_tts_get(char *tts_content)
{
    if (NULL == tts_content) {
        return OPRT_INVALID_PARM;
    }

    char *p_data = NULL;
    int   max_len = strlen(tts_content) + 128;

    if ((p_data = Malloc(max_len)) == NULL) {
        PR_ERR("data print fail");
        return OPRT_COM_ERROR;
    }

    memset(p_data, 0, max_len);
    snprintf(p_data, max_len, "{\"type\":\"getTts\",\"data\":{\"value\":\"%s\"}}", tts_content);
    OPERATE_RET ret = __send_custom_mqtt_msg(p_data);
    Free(p_data);
    return ret;
}

OPERATE_RET tuya_voice_proto_mqtt_devinfo_report(char *devinfo_json)
{
    if (NULL == devinfo_json) {
        return OPRT_INVALID_PARM;
    }

    char *p_data = NULL;
    int  len = strlen(devinfo_json) + 128;

    if ((p_data = Malloc(len)) == NULL) {
        PR_ERR("Malloc failed");
        return OPRT_MALLOC_FAILED;
    }

    memset(p_data, 0, len);
    snprintf(p_data, len, "{\"type\":\"deviceInfo\",\"data\":%s}", devinfo_json);
    OPERATE_RET ret = __send_custom_mqtt_msg(p_data);
    Free(p_data);

    return ret;
}

OPERATE_RET tuya_voice_proto_mqtt_common_report(char *p_data)
{
    if (NULL == p_data) {
        return OPRT_INVALID_PARM;
    }

    return __send_custom_mqtt_msg(p_data);
}

OPERATE_RET tuya_voice_proto_mqtt_thing_config_stop_report(void)
{
    char data[] = "{\"type\":\"distributeNetwork\",\"data\":{\"status\":\"stop\"}}";
    return tuya_voice_proto_mqtt_common_report(data);
}

OPERATE_RET tuya_voice_proto_mqtt_thing_config_request_report(void)
{
    char data[] = "{\"type\":\"distributeNetwork\",\"data\":{\"status\":\"request\"}}";
    return tuya_voice_proto_mqtt_common_report(data);
}

OPERATE_RET tuya_voice_proto_mqtt_thing_config_reject_report(void)
{
    char data[] = "{\"type\":\"distributeNetwork\",\"data\":{\"status\":\"reject\"}}";
    return tuya_voice_proto_mqtt_common_report(data);
}

OPERATE_RET tuya_voice_proto_mqtt_thing_config_access_count_report(int count)
{
    char data[128] = {0};
    sprintf(data, "{\"type\":\"distributeNetwork\",\"data\":{\"status\":\"report\", \"count\":\"%d\"}}", count);
    return tuya_voice_proto_mqtt_common_report(data);
}

OPERATE_RET tuya_voice_proto_mqtt_nick_name_report(TUYA_VOICE_NICK_NAME_OPRT_E oprt, char *nickname, char *pinyin, BOOL_T set_result)
{
    char data[256] = {0};

    if (TUYA_VOICE_NICK_NAME_OPRT_SET == oprt) {
        if(NULL == nickname || NULL == pinyin) {
            PR_ERR("invalid parm");
            return OPRT_INVALID_PARM;
        }
        snprintf(data, 256, "{\"type\":\"nickname\",\"data\":{\"nickname\":\"%s\",\"pinyin\":\"%s\",  \
                                    \"status\":\"save\",\"devSuccess\":%d}}", nickname, pinyin, set_result);
    } else {
        snprintf(data, 256, "{\"type\":\"nickname\",\"data\":{\"status\":\"delete\",\"devSuccess\":%d}}", set_result);
    }

    return tuya_voice_proto_mqtt_common_report(data);
}

OPERATE_RET tuya_voice_proto_mqtt_dndmode_report(BOOL_T set_result, int stamp)
{
    char data[256] = {0};

    snprintf(data, 256, "{\"type\":\"disturb\",\"data\":{\"operation\":\"report\", \"devSucces\":\"%d\", \"stamp\":\"%d\"}}", set_result, stamp);
    return tuya_voice_proto_mqtt_common_report(data);
}

OPERATE_RET tuya_voice_proto_mqtt_dev_status_report(TUYA_VOICE_DEV_STATUS_E status)
{
    char data[128] = {0x0};

    PR_DEBUG("report status:%d", status);
    snprintf(data, sizeof(data), "{\"type\":\"devStatus\",\"data\":{\"statusCode\":%d}}", status);
    return tuya_voice_proto_mqtt_common_report(data);
}

OPERATE_RET tuya_voice_proto_mqtt_online_local_asr_sync(void)
{
    char *data = "{\"type\":\"localAsr\",\"data\":{\"type\":\"localAsr\"}}";
    return tuya_voice_proto_mqtt_common_report(data);
}

OPERATE_RET tuya_voice_proto_mqtt_upload_start(TUYA_VOICE_UPLOAD_T *uploader,
                                    TUYA_VOICE_AUDIO_FORMAT_E format,
                                    TUYA_VOICE_UPLOAD_TARGET_E target,
                                    char *p_session_id,
                                    uint8_t *p_buf,
                                    uint32_t buf_len)
{
    if (NULL == uploader || format >= TUYA_VOICE_AUDIO_FORMAT_INVALD || NULL == p_session_id ||
            (buf_len && NULL == p_buf) || buf_len > TUYA_SPEAKER_MQTT_REPORT_MAX) {
        PR_ERR("invalid parm");
        return OPRT_INVALID_PARM;
    }

    tuya_iot_client_t * iot_client = tuya_iot_client_get();
    if (tuya_iot_is_connected()) {
        PR_ERR("Net Work Unavailable. Can not upload voice...");
        return OPRT_SVC_MQTT_GW_MQ_OFFLILNE;
    }

    TY_VOICE_MQTT_UPLOAD_CTX_S *p_upload_ctx = Malloc(sizeof(TY_VOICE_MQTT_UPLOAD_CTX_S));
    if (NULL == p_upload_ctx) {
        return OPRT_MALLOC_FAILED;
    }

    memset(p_upload_ctx, 0, sizeof(TY_VOICE_MQTT_UPLOAD_CTX_S));
    sprintf(p_upload_ctx->upload_send_topic, "%s%s", MQ_UPLOAD_PUB_TOPIC, iot_client->activate.devid);
    PR_DEBUG("mqtt-upload send topic:%s", p_upload_ctx->upload_send_topic);

    uint8_t *data = NULL;

    SAFE_MALLOC_ERR_RET(data,TUYA_SPEAKER_MQTT_REPORT_MAX + sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S));
    TY_VOICE_MQTT_UPLOAD_DATA_S *p_head = &p_upload_ctx->upload_head;
    TY_VOICE_MQTT_UPLOAD_DATA_S *p_data = (TY_VOICE_MQTT_UPLOAD_DATA_S *)data;
    p_head->version = 0x02;
    p_head->pack_num = 0;
    p_head->upload_flag = TY_UPLOAD_START;
    p_head->req_id = tuya_speaker_get_req_id();
    p_head->message_id = tal_time_get_posix();
    PR_DEBUG("message_id: %lld", p_head->message_id);
    p_head->voice_encode = format;
    p_head->target = target;
    strncpy(p_head->session_id, p_session_id, sizeof(p_head->session_id));
    p_head->data_len = buf_len;

    __pack_upload_data(p_head, p_data);
    if (buf_len) {
        memcpy(p_data->data, p_buf, buf_len);   /**< FIXME: compatible wss new  encode media codec_speex & codec_opus */
    }

    // tuya_iot_client_t *iot_client = tuya_iot_client_get();
    OPERATE_RET ret = tuya_mqtt_client_publish_common(&iot_client->mqctx, p_upload_ctx->upload_send_topic, (uint8_t *)p_data, 
                            buf_len + sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S), NULL, NULL, 2, TRUE);
    // OPERATE_RET ret = tuya_iot_custom_data_report_async((uint8_t *)p_data, buf_len + sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S),
    //                   p_upload_ctx->upload_send_topic, 0, 2);
    if (ret != OPRT_OK) {
        PR_ERR("data report fail: %d", ret);
        Free(p_upload_ctx);
        SAFE_FREE(data);
        return ret;
    }

    *uploader = p_upload_ctx;
    SAFE_FREE(data);
    PR_DEBUG("upload media start. media_encode:%d session_id:%s -->> target:%d", format, p_head->session_id, target);
    return OPRT_OK;
}

OPERATE_RET tuya_voice_proto_mqtt_upload_send(TUYA_VOICE_UPLOAD_T uploader, uint8_t *buf, uint32_t len)
{
    if (NULL == uploader || (len && !buf)) {
        return OPRT_INVALID_PARM;
    }

    tuya_iot_client_t *iot_client = tuya_iot_client_get();
    TY_VOICE_MQTT_UPLOAD_CTX_S *p_upload_ctx = (TY_VOICE_MQTT_UPLOAD_CTX_S *)uploader;
    uint8_t *data = NULL;

    SAFE_MALLOC_ERR_RET(data,TUYA_SPEAKER_MQTT_REPORT_MAX + sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S));


    TY_VOICE_MQTT_UPLOAD_DATA_S *p_head = &p_upload_ctx->upload_head;
    TY_VOICE_MQTT_UPLOAD_DATA_S *p_data = (TY_VOICE_MQTT_UPLOAD_DATA_S *)data;
    int sent_count = 0, count;

    while (len > sent_count) {
        memset(data,0,TUYA_SPEAKER_MQTT_REPORT_MAX + sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S));
        count = ((len - sent_count) > TUYA_SPEAKER_MQTT_REPORT_MAX) ?
                TUYA_SPEAKER_MQTT_REPORT_MAX : (len - sent_count);
        p_head->pack_num ++;
        p_head->upload_flag = TY_UPLOAD_MID;
        p_head->data_len = count;

        __pack_upload_data(p_head, p_data);
        memcpy(p_data->data, buf + sent_count, count);
        
        OPERATE_RET ret = tuya_mqtt_client_publish_common(&iot_client->mqctx, p_upload_ctx->upload_send_topic, (uint8_t *)p_data, 
                            count + sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S), NULL, NULL, 2, TRUE);
        // OPERATE_RET ret = tuya_iot_custom_data_report_async((uint8_t *)p_data, count + sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S),
        //                   p_upload_ctx->upload_send_topic, 0, 2);
        PR_TRACE("upload media %d ---", count);
        if (ret != OPRT_OK) {
            PR_ERR("upload media fail.len:%d ret:%d", count, ret);
            SAFE_FREE(data);
            return OPRT_COM_ERROR;
        }

        sent_count += count;
        p_upload_ctx->data_len += count;
    }
    SAFE_FREE(data);
    return OPRT_OK;
}

OPERATE_RET tuya_voice_proto_mqtt_upload_stop(TUYA_VOICE_UPLOAD_T uploader, BOOL_T force_stop)
{
    OPERATE_RET ret = OPRT_OK;

    if (NULL == uploader) {
        return OPRT_INVALID_PARM;
    }

    tuya_iot_client_t *iot_client = tuya_iot_client_get();
    TY_VOICE_MQTT_UPLOAD_CTX_S *p_upload_ctx = (TY_VOICE_MQTT_UPLOAD_CTX_S *)uploader;
    if (!force_stop) {
        TY_VOICE_MQTT_UPLOAD_DATA_S data, *p_head = &p_upload_ctx->upload_head;
        p_head->pack_num ++;
        p_head->upload_flag = TY_UPLOAD_END;
        p_head->data_len = 0;

        __pack_upload_data(p_head, &data);
        ret = tuya_mqtt_client_publish_common(&iot_client->mqctx, p_upload_ctx->upload_send_topic, (uint8_t *)&data, 
                            sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S), NULL, NULL, 2, TRUE);
        // ret = tuya_iot_custom_data_report_async((uint8_t *)&data, sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S),
        //                                         p_upload_ctx->upload_send_topic, 0, 2);
    }

    PR_DEBUG("upload media stop ret:%d data_len:%d force_stop:%d --<<", ret, p_upload_ctx->data_len, force_stop);
    Free(p_upload_ctx);
    return ret;
}

OPERATE_RET tuya_voice_proto_mqtt_upload_get_message_id(TUYA_VOICE_UPLOAD_T uploader, char *buffer, int len)
{
    if (NULL == uploader) {
        return OPRT_INVALID_PARM;
    }

    TY_VOICE_MQTT_UPLOAD_CTX_S *p_upload_ctx = (TY_VOICE_MQTT_UPLOAD_CTX_S *)uploader;
    TY_VOICE_MQTT_UPLOAD_DATA_S *p_head = &p_upload_ctx->upload_head;

    snprintf(buffer, len, "%lld", p_head->message_id);

    return OPRT_OK;
}

static uint32_t tuya_speaker_get_req_id()
{
    static uint32_t req_id = SPEAKER_UPLOAD_ID_MIN;

    req_id = (req_id >= SPEAKER_UPLOAD_ID_MAX) ? SPEAKER_UPLOAD_ID_MIN : (req_id + 1);
    PR_DEBUG("req id:%d", req_id);
    return req_id;
}

static OPERATE_RET __pack_upload_data(TY_VOICE_MQTT_UPLOAD_DATA_S *p_in, TY_VOICE_MQTT_UPLOAD_DATA_S *p_out)
{
    if (NULL == p_in || NULL == p_out) {
        PR_ERR("invalid parm");
        return OPRT_INVALID_PARM;
    }

    memset(p_out, 0x0, sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S));
    memcpy(p_out, p_in, sizeof(TY_VOICE_MQTT_UPLOAD_DATA_S));
    p_out->pack_num = UNI_HTONL(p_in->pack_num);
    p_out->req_id = UNI_HTONL(p_in->req_id);
    p_out->data_len = UNI_HTONL(p_in->data_len);
    UNI_HTONLL(p_out->message_id);

    //PR_DEBUG("in message_id %lld, out message_id %lld", p_in->message_id, p_out->message_id);

    return OPRT_OK;
}

static OPERATE_RET __send_custom_mqtt_msg(char *p_data)
{
    PR_DEBUG("send data:%s", p_data);
    tuya_iot_client_t *iot_client = tuya_iot_client_get();
    return tuya_mqtt_protocol_data_publish_common(&iot_client->mqctx, VOICE_MQ_PROTOCOL_NUM, p_data, strlen(p_data), NULL, NULL, 0, TRUE);
    // return iot_mqc_send_custom_msg(VOICE_MQ_PROTOCOL_NUM, p_data, 0, 0, NULL, NULL);
}

static void __result_cb(OPERATE_RET op_ret, void *prv_data)
{
    PR_DEBUG("mqtt report result: %d", op_ret);
    SYNC_REPORT *p_report = (SYNC_REPORT *)prv_data;
    p_report->op_ret = op_ret;
    tal_semaphore_post(p_report->sem_handle);
}

static OPERATE_RET __send_custom_mqtt_msg_wait(char *p_data, int overtime_s)
{
    PR_DEBUG("send data:%s overtime:%d", p_data, overtime_s);

    tuya_iot_client_t *iot_client = tuya_iot_client_get();
    SYNC_REPORT *p_report = Malloc(sizeof(SYNC_REPORT));
    if (NULL == p_report) {
        PR_ERR("Malloc failed");
        return OPRT_MALLOC_FAILED;
    }

    OPERATE_RET ret = tal_semaphore_create_init(&(p_report->sem_handle), 0, 10);
    if (ret != OPRT_OK) {
        PR_ERR("create semaphore failed %d", ret);
        Free(p_report);
        return ret;
    }

    p_report->op_ret = 100;
    ret = tuya_mqtt_protocol_data_publish_common(&iot_client->mqctx, VOICE_MQ_PROTOCOL_NUM, p_data, strlen(p_data), __result_cb, p_report, overtime_s, TRUE);
    // ret = iot_mqc_send_custom_msg(VOICE_MQ_PROTOCOL_NUM, p_data, 1, overtime_s, __result_cb, p_report);
    if (ret != OPRT_OK) {
        PR_ERR("send custom msg fail. %d", ret);
        tal_semaphore_release(p_report->sem_handle);
        Free(p_report);
        return ret;
    }


    tal_semaphore_wait(p_report->sem_handle,SEM_WAIT_FOREVER);
    if (p_report->op_ret == 100) {
        p_report->op_ret = OPRT_COM_ERROR;
    }

    ret = p_report->op_ret;
    tal_semaphore_release(p_report->sem_handle);

    PR_DEBUG("send finish. ret:%d", p_report->op_ret);
    Free(p_report);
    return ret;
}

static OPERATE_RET __parse_cloud_thing_config(cJSON *json)
{
    char          *token = NULL;
    uint32_t           timeout = 0;

    cJSON *obj_operate = cJSON_GetObjectItem(json, "status");
    if (NULL == obj_operate) {
        PR_ERR("input is invalid");
        return OPRT_INVALID_PARM;
    }

    if (!strcmp(obj_operate->valuestring, "notify")) {
        cJSON *obj_count = cJSON_GetObjectItem(json, "count");
        int count = obj_count->valueint;

        if (g_voice_mqtt_cbs.tuya_voice_subdev_access) {
            g_voice_mqtt_cbs.tuya_voice_subdev_access(count);
        }

        return OPRT_OK;
    }

    TUYA_VOICE_THING_CONFIG_MODE_E mode = TUYA_VOICE_THING_CONFIG_STOP;
    if (strcmp(obj_operate->valuestring, "start") == 0) {
        mode = TUYA_VOICE_THING_CONFIG_START;
    }

    if (TUYA_VOICE_THING_CONFIG_START == mode) {
        cJSON *obj_token = cJSON_GetObjectItem(json, "token");
        cJSON *obj_timeout = cJSON_GetObjectItem(json, "timeout");

        if (NULL == obj_token || NULL == obj_timeout) {
            PR_ERR("input is invalid");
            return OPRT_INVALID_PARM;
        }

        token = obj_token->valuestring;
        timeout = obj_timeout->valueint;
        PR_DEBUG("token:%s timeout:%d", token, timeout);
    }

    if (g_voice_mqtt_cbs.tuya_voice_thing_config) {
        g_voice_mqtt_cbs.tuya_voice_thing_config(mode, token, timeout);
    }

    return OPRT_OK;
}

static OPERATE_RET __parse_cloud_nick_name(cJSON *json)
{
    cJSON *obj_nickname = cJSON_GetObjectItem(json, "nickname");
    cJSON *obj_pinyin = cJSON_GetObjectItem(json, "pinyin");
    cJSON *obj_operate = cJSON_GetObjectItem(json, "status");

    if (NULL == obj_operate) {
        PR_ERR("obj_operate not found");
        return OPRT_INVALID_PARM;
    }

    TUYA_VOICE_NICK_NAME_OPRT_E oprt = TUYA_VOICE_NICK_NAME_OPRT_DEL;
    if (strcmp(obj_operate->valuestring, "save") == 0) {
        oprt = TUYA_VOICE_NICK_NAME_OPRT_SET;
    }

    if (TUYA_VOICE_NICK_NAME_OPRT_SET == oprt) {
        if (NULL == obj_nickname || NULL == obj_pinyin) {
            PR_ERR("input is invalid");
            return OPRT_INVALID_PARM;
        }

        g_voice_mqtt_cbs.tuya_voice_nick_name(oprt, obj_nickname->valuestring, obj_pinyin->valuestring);
    } else {
        g_voice_mqtt_cbs.tuya_voice_nick_name(oprt, NULL, NULL);
    }

    return OPRT_OK;
}

static OPERATE_RET __parse_cloud_dnd_mode_cb(cJSON *json)
{
    if (NULL == json) {
        PR_ERR("param is invalid");
        return OPRT_INVALID_PARM;
    }

    cJSON *obj_state = cJSON_GetObjectItem(json, "state");
    cJSON *obj_start_time = cJSON_GetObjectItem(json, "startTime");
    cJSON *obj_end_time = cJSON_GetObjectItem(json, "endTime");
    cJSON *obj_stamp = cJSON_GetObjectItem(json, "endTime");

    if (NULL == obj_state || NULL == obj_start_time ||  NULL == obj_end_time) {
        PR_ERR("obj error, obj_state:%p, obj_start_time:%p, obj_end_time:%p", \
            obj_state, obj_start_time, obj_end_time);
        return OPRT_COM_ERROR;
    }

    g_voice_mqtt_cbs.tuya_voice_dnd_mode((BOOL_T)obj_state->valueint, obj_start_time->valuestring,
                                          obj_end_time->valuestring, obj_stamp->valueint);

    return OPRT_OK;
}

static OPERATE_RET __parse_cloud_call_operate(TUYA_VOICE_TEL_MODE_T mode, cJSON *json)
{
    if (g_voice_mqtt_cbs.tuya_voice_tel_operate) {
        g_voice_mqtt_cbs.tuya_voice_tel_operate(mode);
    }

    return OPRT_OK;
}

static OPERATE_RET __parse_cloud_call_second_dial(cJSON *json)
{
    if (g_voice_mqtt_cbs.tuya_voice_call_second_dial) {
        cJSON *obj_dial = cJSON_GetObjectItem(json, "dial");
        if (NULL == obj_dial) {
            PR_ERR("obj_dial not found");
            return OPRT_INVALID_PARM;
        }

        g_voice_mqtt_cbs.tuya_voice_call_second_dial(obj_dial->valuestring);
    }

    return OPRT_OK;
}


static OPERATE_RET __voice_mqc_proto_cb_cb(cJSON *root_json)
{
    cJSON   *json = NULL, *sub_json = NULL, *type = NULL;
    OPERATE_RET rt;
    if ((json = cJSON_GetObjectItem(root_json, "data")) == NULL) {
        PR_ERR("data not in rootJson");
        return OPRT_CJSON_PARSE_ERR;
    }

    type     = cJSON_GetObjectItem(json, "type");
    sub_json = cJSON_GetObjectItem(json, "data");

    if (NULL == sub_json || NULL == type) {
        char *p_dump = cJSON_PrintUnformatted(json);
        PR_ERR("dump:%s", p_dump);
        Free(p_dump);
        PR_ERR("not found data");
        return OPRT_CJSON_PARSE_ERR;
    }

    if (!strcmp(type->valuestring, "playTts") && g_voice_mqtt_cbs.tuya_voice_play_tts != NULL) {
        TUYA_VOICE_TTS_S *tts = NULL;
        if (tuya_voice_json_parse_tts(sub_json, &tts) != OPRT_OK) {
            PR_ERR("parse tts error");
            return OPRT_COM_ERROR;
        }
        g_voice_mqtt_cbs.tuya_voice_play_tts(tts);
        tuya_voice_json_parse_free_tts(tts);
    } else if (!strcmp(type->valuestring, "playAudio") && g_voice_mqtt_cbs.tuya_voice_play_audio != NULL) {
        TUYA_VOICE_MEDIA_S *media = NULL;
        if (tuya_voice_json_parse_media(sub_json, &media) != OPRT_OK) {
            PR_ERR("parse audio error");
            return OPRT_COM_ERROR;
        }
        g_voice_mqtt_cbs.tuya_voice_play_audio(media);
        tuya_voice_json_parse_free_media(media);
    } else if (!strcmp(type->valuestring, "syncAudioRequest") && g_voice_mqtt_cbs.tuya_voice_audio_sync != NULL) {
        g_voice_mqtt_cbs.tuya_voice_audio_sync();
        return OPRT_OK;
    } else if (!strcmp(type->valuestring, "distributeNetwork") && g_voice_mqtt_cbs.tuya_voice_thing_config != NULL) {
        TUYA_CALL_ERR_RETURN(__parse_cloud_thing_config(sub_json));
    } else if (!strcmp(type->valuestring, "nickname") && g_voice_mqtt_cbs.tuya_voice_nick_name != NULL) {
        TUYA_CALL_ERR_RETURN(__parse_cloud_nick_name(sub_json));
    } else if (!strcmp(type->valuestring, "disturb") && g_voice_mqtt_cbs.tuya_voice_dnd_mode != NULL) {
        TUYA_CALL_ERR_RETURN(__parse_cloud_dnd_mode_cb(sub_json));
    } else if(!strcmp(type->valuestring, "answerAiCall") && g_voice_mqtt_cbs.tuya_voice_tel_operate != NULL) {
        __parse_cloud_call_operate(TUYA_VOICE_TEL_MODE_ANSWER, sub_json);
    } else if(!strcmp(type->valuestring, "refuseAiCall") && g_voice_mqtt_cbs.tuya_voice_tel_operate != NULL) {
        __parse_cloud_call_operate(TUYA_VOICE_TEL_MODE_REFUSE, sub_json);
    } else if(!strcmp(type->valuestring, "hangUpAiCall") && g_voice_mqtt_cbs.tuya_voice_tel_operate != NULL) {
        __parse_cloud_call_operate(TUYA_VOICE_TEL_MODE_HANGUP, sub_json);
    } else if(!strcmp(type->valuestring, "telephoneBind") && g_voice_mqtt_cbs.tuya_voice_tel_operate != NULL) {
        __parse_cloud_call_operate(TUYA_VOICE_TEL_MODE_BIND, sub_json);
    } else if(!strcmp(type->valuestring, "telephoneUnbind") && g_voice_mqtt_cbs.tuya_voice_tel_operate != NULL) {
        __parse_cloud_call_operate(TUYA_VOICE_TEL_MODE_UNBIND, sub_json);
    } else if(!strcmp(type->valuestring, "callSecondDial") && g_voice_mqtt_cbs.tuya_voice_call_second_dial != NULL) {
        __parse_cloud_call_second_dial(sub_json);
    } else if(!strcmp(type->valuestring, "callPhoneV2") && g_voice_mqtt_cbs.tuya_voice_call_phone_v2 != NULL) {
        TUYA_VOICE_CALL_PHONE_INFO_S *call_info = NULL;
        if (tuya_voice_json_parse_call_info(sub_json, &call_info) != OPRT_OK) {
            PR_ERR("parse call info error");
            return OPRT_COM_ERROR;
        }
        g_voice_mqtt_cbs.tuya_voice_call_phone_v2(call_info);
        tuya_voice_json_parse_free_call_info(call_info);
    }else {
        PR_ERR("start custom cb, type:%s", type->valuestring);
        if (g_voice_mqtt_cbs.tuya_voice_custom != NULL) {
            g_voice_mqtt_cbs.tuya_voice_custom(type->valuestring, sub_json);
        }
        return OPRT_OK;
    }

    PR_DEBUG("mqtt rev speaker called finish <<--");

    return OPRT_OK;
}

static void __voice_mqc_proto_cb(tuya_protocol_event_t* ev)
{
    __voice_mqc_proto_cb_cb(ev->root_json);
}