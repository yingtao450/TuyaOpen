/**
 * @file ai_audio_main.c
 * @version 0.1
 * @date 2025-04-15
 */

#include "tuya_cloud_types.h"
#include "tkl_audio.h"
#include "tkl_queue.h"
#include "tkl_memory.h"
#include "tkl_thread.h"
#include "tkl_asr.h"

#include "tal_api.h"
#include "ai_audio.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define AI_AUDIO_SPEAK_VOLUME_KEY "spk_volume"

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    uint32_t frame_len;
    uint8_t *frame;
} AI_AUDIO_FRAME_MSG_T;

/***********************************************************
********************function declaration********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
static bool sg_ai_audio_is_silent = true;
static AI_AGENT_MSG_CB sg_ai_agent_msg_cb = NULL;
static bool sg_is_session_continue = false;
static bool sg_is_ai_speaking = false;
/***********************************************************
***********************function define**********************
***********************************************************/
static void __ai_audio_agent_msg_cb(AI_AGENT_MSG_T *msg)
{

    switch (msg->type) {
    case AI_AGENT_MSG_TP_TEXT_ASR: {
        if (msg->data_len > 0) {
            // Prepare to play mp3
            if (ai_audio_player_is_playing()) {
                PR_DEBUG("player is playing, stop it first");
                ai_audio_player_stop();
            }
            ai_audio_player_start();
        }

        if (AI_CLOUD_ASR_STATE_WAIT_ASR == ai_audio_cloud_asr_get_state()) {
            PR_DEBUG("get cloud asr, set idle");
            ai_audio_cloud_asr_idle();
        }

    } break;
    case AI_AGENT_MSG_TP_AUDIO_DATA: {
        sg_is_ai_speaking = true;
        ai_audio_player_data_write(msg->data, msg->data_len, 0);
    } break;
    case AI_AGENT_MSG_TP_AUDIO_STOP: {
        ai_audio_player_data_write(msg->data, msg->data_len, 1);

        if (AI_AUDIO_INPUT_WAKEUP_ASR == ai_audio_input_get_wakeup_tp()) {
            if (false == sg_is_session_continue) {
                ai_audio_input_restart_asr_detect_wakeup_word();
            } else {
                ai_audio_input_trigger_asr_awake();
            }
        }
        sg_is_ai_speaking = false;
    } break;
    default:
        break;
    }

    if (sg_ai_agent_msg_cb) {
        sg_ai_agent_msg_cb(msg);
    }
}

static void __ai_audio_input_inform_handle(AI_AUDIO_INPUT_EVENT_E event, uint8_t *data, uint32_t len, void *arg)
{
    static bool is_need_filter = false;

    switch (event) {
    case AI_AUDIO_INPUT_EVT_IDLE:
        ai_audio_cloud_asr_rb_reset();
        ai_audio_cloud_asr_stop();
        break;
    case AI_AUDIO_INPUT_EVT_ENTER_DETECT:
        ai_audio_cloud_asr_input(data, len);
        ai_audio_cloud_asr_stop();
        break;
    case AI_AUDIO_INPUT_EVT_DETECTING:
        ai_audio_cloud_asr_vad_input(data, len);
        break;
    case AI_AUDIO_INPUT_EVT_ASR_WORD:
        if (sg_is_ai_speaking) {
            PR_NOTICE("intrrupt");
            ai_audio_agent_upload_intrrupt();
        }

        ai_audio_player_play_alert_syn(AI_AUDIO_ALERT_WAKEUP);
        ai_audio_input_filter_player_voice(true);
        is_need_filter = true;
        break;
    case AI_AUDIO_INPUT_EVT_WAKEUP: {
        TKL_ASR_WAKEUP_WORD_E key_word = TKL_ASR_WAKEUP_WORD_UNKNOWN;

        if (is_need_filter) {
            ai_audio_input_filter_player_voice(false);
            is_need_filter = false;
        }

        ai_audio_cloud_asr_input(data, len);
        ai_audio_cloud_asr_start();
    } break;
    case AI_AUDIO_INPUT_EVT_AWAKE:
        if (sg_is_ai_speaking) {
            ai_audio_cloud_asr_stop();
        } else {
            ai_audio_cloud_asr_input(data, len);
        }
        break;
    }
}

OPERATE_RET ai_audio_init(AI_AUDIO_CONFIG_T *cfg)
{
    OPERATE_RET rt = OPRT_OK;
    AI_AUDIO_INPUT_CFG_T input_cfg;

    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    input_cfg.wakeup_tp = cfg->wakeup_tp;
    input_cfg.asr_wakeup_timeout_ms = cfg->asr_wakeup_timeout_ms;
    sg_is_session_continue = cfg->is_session_continue;

    TUYA_CALL_ERR_RETURN(ai_audio_input_init(&input_cfg, __ai_audio_input_inform_handle));

    TUYA_CALL_ERR_RETURN(tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, 0, NULL, ai_audio_get_volume()));

    TUYA_CALL_ERR_RETURN(ai_audio_cloud_asr_init(cfg->is_enable_interrupt));

    TUYA_CALL_ERR_RETURN(ai_audio_player_init());

    TUYA_CALL_ERR_RETURN(ai_audio_agent_init(__ai_audio_agent_msg_cb));
    sg_ai_agent_msg_cb = cfg->agent_msg_cb;

    return OPRT_OK;
}

OPERATE_RET ai_audio_set_volume(uint8_t volume)
{
    OPERATE_RET rt = OPRT_OK;

    // kv storage
    TUYA_CALL_ERR_LOG(tal_kv_set(AI_AUDIO_SPEAK_VOLUME_KEY, &volume, sizeof(volume)));
    TUYA_CALL_ERR_LOG(tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, 0, NULL, volume));

    return rt;
}

uint8_t ai_audio_get_volume(void)
{
    OPERATE_RET rt = OPRT_OK;

    uint8_t volume = 0;
    uint8_t *value = NULL;
    size_t read_len = 0;

    // kv read
    TUYA_CALL_ERR_LOG(tal_kv_get(AI_AUDIO_SPEAK_VOLUME_KEY, &value, &read_len));
    if (OPRT_OK != rt || NULL == value) {
        PR_ERR("read volume failed");
        volume = 50;
    } else {
        volume = *value;
    }

    PR_DEBUG("get spk volume: %d", volume);

    if (value) {
        tal_kv_free(value);
        value = NULL;
    }

    return volume;
}

OPERATE_RET ai_audio_set_silent(bool is_silent)
{
    if (true == is_silent) {
        ai_audio_input_close();
    } else {
        ai_audio_input_open();
    }

    sg_ai_audio_is_silent = is_silent;

    return OPRT_OK;
}

bool ai_audio_is_silent(void)
{
    return sg_ai_audio_is_silent;
}
