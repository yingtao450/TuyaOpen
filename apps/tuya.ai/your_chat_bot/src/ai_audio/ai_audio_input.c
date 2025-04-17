/**
 * @file ai_audio_input.c
 * @version 0.1
 * @date 2025-04-17
 */
#include "tkl_asr.h"
#include "tkl_audio.h"
#include "tkl_vad.h"
#include "tkl_memory.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"

#include "tal_api.h"
#include "tuya_ringbuf.h"

#include "ai_audio_input.h"

#include "ty_vad_app.h"
/***********************************************************
************************macro define************************
***********************************************************/
#define INPUT_RINGBUFF_LEN (1024 * 2)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    uint8_t is_init;
    uint8_t is_open_vad;
    uint8_t is_enable_interrupt;

    uint8_t is_open_asr;
    uint8_t *asr_buff;
    uint32_t asr_buff_len;
    uint32_t asr_used_len;

    TIMER_ID wakeup_timer_id;
    uint32_t wakeup_timeout_ms;

    TUYA_RINGBUFF_T ringbuff_hdl;
    MUTEX_HANDLE rb_mutex;

    AI_AUDIO_INPUT_STATE_E state;
} AI_AUDIO_INPUT_INFO_T;

/***********************************************************
***********************const declaration********************
***********************************************************/
const static TKL_ASR_WAKEUP_WORD_E cWAKEUP_KEYWORD_LIST[] = {
    TKL_ASR_WAKEUP_NIHAO_TUYA,
};

/***********************************************************
***********************variable define**********************
***********************************************************/
static AI_AUDIO_INOUT_INFORM_CB sg_audio_input_inform_cb = NULL;
static THREAD_HANDLE sg_ai_audio_input_thrd_hdl = NULL;
static AI_AUDIO_INPUT_INFO_T sg_audio_input;
/***********************************************************
***********************function define**********************
***********************************************************/
static void __ai_audio_input_enter_idle(TIMER_ID timer_id, void *arg)
{
    if (sg_audio_input.is_open_vad) {
        sg_audio_input.state = AI_AUDIO_INPUT_STATE_DETECTING;
    } else {
        sg_audio_input.state = AI_AUDIO_INPUT_STATE_IDLE;
    }
    sg_audio_input_inform_cb(sg_audio_input.state, NULL, 0, NULL);
}

static int __ai_audio_get_input_frame(TKL_AUDIO_FRAME_INFO_T *pframe)
{
    OPERATE_RET rt = OPRT_OK;

    if (AI_AUDIO_INPUT_STATE_IDLE == sg_audio_input.state) {
        return 0;
    }

    if (false == sg_audio_input.is_enable_interrupt) {
        if (ai_audio_player_is_playing()) {
            return 0;
        }
    }

    tal_mutex_lock(sg_audio_input.rb_mutex);
    tuya_ring_buff_write(sg_audio_input.ringbuff_hdl, pframe->pbuf, pframe->used_size);
    tal_mutex_unlock(sg_audio_input.rb_mutex);

    return rt;
}

static void __ai_audio_wakeup_asr_feed(void *data, uint32_t len)
{
    uint32_t asr_free_len = 0;

    asr_free_len = sg_audio_input.asr_buff_len - sg_audio_input.asr_used_len;

    if (len > asr_free_len) {
        PR_ERR("asr buff remain len:%d is not enough len:%d", asr_free_len, len);
        return;
    }

    memcpy(sg_audio_input.asr_buff + sg_audio_input.asr_used_len, data, len);
    sg_audio_input.asr_used_len += len;

    return;
}

static void __ai_audio_wake_asr_reset(void)
{
    sg_audio_input.asr_used_len = 0;
}

static TKL_ASR_WAKEUP_WORD_E __asr_recognize_wakeup_keyword(void)
{
    uint32_t i = 0, fc = 0;
    TKL_ASR_WAKEUP_WORD_E wakeup_word = TKL_ASR_WAKEUP_WORD_UNKNOWN;
    uint32_t uint_size = tkl_asr_get_process_uint_size();

    if (sg_audio_input.asr_used_len < uint_size) {
        PR_DEBUG("wakeup data:%d not enough %d", sg_audio_input.asr_used_len, uint_size);
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    fc = sg_audio_input.asr_used_len / uint_size;
    for (i = 0; i < fc; i++) {
        wakeup_word = tkl_asr_recognize_wakeup_word(sg_audio_input.asr_buff + i * uint_size, uint_size);
        if (wakeup_word != TKL_ASR_WAKEUP_WORD_UNKNOWN) {
            sg_audio_input.asr_used_len = 0;
            break;
        }
        sg_audio_input.asr_used_len -= uint_size;
    }

    if (sg_audio_input.asr_used_len) {
        memmove(sg_audio_input.asr_buff, sg_audio_input.asr_buff + fc * uint_size, sg_audio_input.asr_used_len);
    }

    return wakeup_word;
}

AI_AUDIO_INPUT_EVENT_E __ai_audio_input_detect_event(AI_AUDIO_INPUT_STATE_E curr_state)
{
    AI_AUDIO_INPUT_EVENT_E event = AI_AUDIO_INPUT_EVT_IDLE;
    TKL_ASR_WAKEUP_WORD_E key = TKL_ASR_WAKEUP_WORD_UNKNOWN;

    if (AI_AUDIO_INPUT_STATE_IDLE == curr_state) {
        return AI_AUDIO_INPUT_EVT_IDLE;
    }

    if (false == sg_audio_input.is_open_vad) {
        if (curr_state == AI_AUDIO_INPUT_STATE_IDLE) {
            event = AI_AUDIO_INPUT_EVT_IDLE;
        } else {
            event = AI_AUDIO_INPUT_EVT_AWAKE;
        }

        return event;
    }

#if 0
    if (TKL_VAD_STATUS_SPEECH == tkl_vad_get_status()) {
#else
    if (TY_VAD_FLAG_VAD_START == ty_get_vad_flag()) {
#endif
        if (sg_audio_input.is_open_asr) {
            key = __asr_recognize_wakeup_keyword();
            if (TKL_ASR_WAKEUP_WORD_UNKNOWN != key) {
                event = AI_AUDIO_INPUT_EVT_WAKEUP;
            } else {
                if (AI_AUDIO_INPUT_STATE_AWAKE == curr_state) {
                    event = AI_AUDIO_INPUT_EVT_AWAKE;
                } else {
                    event = AI_AUDIO_INPUT_EVT_DETECTING;
                }
            }
        } else {
            if (AI_AUDIO_INPUT_STATE_AWAKE == curr_state) {
                event = AI_AUDIO_INPUT_EVT_AWAKE;
            } else {
                event = AI_AUDIO_INPUT_EVT_WAKEUP;
            }
        }
    } else {
        if (sg_audio_input.is_open_asr) {
            __ai_audio_wake_asr_reset();
        }

        if (AI_AUDIO_INPUT_STATE_DETECTING == curr_state) {
            event = AI_AUDIO_INPUT_EVT_DETECTING;
        } else {
            event = AI_AUDIO_INPUT_EVT_ENTER_DETECT;
        }
    }

    return event;
}

static void __ai_audio_wakeup_feed(uint8_t *data, uint32_t len)
{
    if (sg_audio_input.is_open_vad) {
#if 0
        tkl_vad_feed(data, len);
#else
        for (int i = 0; i < (len / 320); i++) {
            ty_vad_frame_put(data + i * 320, 320);
        }
        // ty_vad_frame_put(data, len);
#endif
    }

    if (sg_audio_input.is_open_asr) {
        __ai_audio_wakeup_asr_feed(data, len);
    }
}
void __update_input_state_depend_on_event(AI_AUDIO_INPUT_EVENT_E event)
{
    switch (event) {
    case AI_AUDIO_INPUT_EVT_IDLE:
        if (tal_sw_timer_is_running(sg_audio_input.wakeup_timer_id)) {
            tal_sw_timer_stop(sg_audio_input.wakeup_timer_id);
        }
        sg_audio_input.state = AI_AUDIO_INPUT_STATE_IDLE;
        break;
    case AI_AUDIO_INPUT_EVT_ENTER_DETECT:
        if (tal_sw_timer_is_running(sg_audio_input.wakeup_timer_id)) {
            tal_sw_timer_stop(sg_audio_input.wakeup_timer_id);
        }

        sg_audio_input.state = AI_AUDIO_INPUT_STATE_DETECTING;
        break;
    case AI_AUDIO_INPUT_EVT_WAKEUP:
        sg_audio_input.state = AI_AUDIO_INPUT_STATE_AWAKE;
        if (sg_audio_input.wakeup_timeout_ms) {
            tal_sw_timer_start(sg_audio_input.wakeup_timer_id, sg_audio_input.wakeup_timeout_ms, TAL_TIMER_ONCE);
        }
        break;
    default:
        break;
    }
}

static void __ai_audio_handle_frame_task(void *arg)
{
    OPERATE_RET rt = OPRT_OK;
    uint32_t rb_used_sz = 0;
    uint8_t *p_buff = NULL;
    AI_AUDIO_INPUT_EVENT_E event = AI_AUDIO_INPUT_EVT_IDLE;
    TKL_ASR_WAKEUP_WORD_E wakeup_word = TKL_ASR_WAKEUP_WORD_UNKNOWN;

    while (1) {
        rb_used_sz = tuya_ring_buff_used_size_get(sg_audio_input.ringbuff_hdl);
        if (0 == rb_used_sz) {
            tkl_system_sleep(10);
            continue;
        }

        p_buff = tkl_system_psram_malloc(rb_used_sz);
        if (NULL == p_buff) {
            PR_DEBUG("malloc failed");
            tkl_system_sleep(100);
            continue;
        }

        tal_mutex_lock(sg_audio_input.rb_mutex);
        tuya_ring_buff_read(sg_audio_input.ringbuff_hdl, p_buff, rb_used_sz);
        tal_mutex_unlock(sg_audio_input.rb_mutex);

        __ai_audio_wakeup_feed(p_buff, rb_used_sz);

        event = __ai_audio_input_detect_event(sg_audio_input.state);

        __update_input_state_depend_on_event(event);

        if (sg_audio_input_inform_cb) {
            void *arg = (AI_AUDIO_INPUT_EVT_WAKEUP == event) ? (void *)&wakeup_word : NULL;
            sg_audio_input_inform_cb(event, p_buff, rb_used_sz, arg);
        }

        tkl_system_psram_free(p_buff);
        p_buff = NULL;

        tal_system_sleep(5);
    }
}

static OPERATE_RET __ai_audio_input_hardware_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    TKL_AUDIO_CONFIG_T config;
    memset(&config, 0, sizeof(TKL_AUDIO_CONFIG_T));

    config.ai_chn = TKL_AI_0;
    config.sample = TKL_AUDIO_SAMPLE_16K;
    config.datebits = TKL_AUDIO_DATABITS_16;
    config.channel = TKL_AUDIO_CHANNEL_MONO;
    config.codectype = TKL_CODEC_AUDIO_PCM;
    config.card = TKL_AUDIO_TYPE_BOARD;
    config.put_cb = __ai_audio_get_input_frame;

    config.spk_sample = TKL_AUDIO_SAMPLE_16K;
    config.spk_gpio = SPEAKER_EN_PIN;
    config.spk_gpio_polarity = TUYA_GPIO_LEVEL_LOW;

    TUYA_CALL_ERR_RETURN(tkl_ai_init(&config, 0));
    TUYA_CALL_ERR_RETURN(tkl_ai_start(0, 0));
    TUYA_CALL_ERR_RETURN(tkl_ai_set_vol(TKL_AUDIO_TYPE_BOARD, 0, 80));

    PR_DEBUG("__ai_audio_input_hardware_init success");

    return OPRT_OK;
}

#if 0
static OPERATE_RET __ai_audio_open_vad(void)
{
    OPERATE_RET rt = OPRT_OK;

    TKL_VAD_CONFIG_T vad_config;
    vad_config.sample_rate = TKL_AUDIO_SAMPLE_16K;
    vad_config.channel_num = TKL_AUDIO_CHANNEL_MONO;
    vad_config.speech_min_ms = 300;
    vad_config.noise_min_ms = 500;
    vad_config.scale = 2.5;
    vad_config.frame_duration_ms = 10;

    TUYA_CALL_ERR_RETURN(tkl_vad_init(&vad_config));
    TUYA_CALL_ERR_RETURN(tkl_vad_start());

    return OPRT_OK;
}
#else
static OPERATE_RET __ai_audio_open_vad(void)
{
    OPERATE_RET rt = OPRT_OK;

    ty_vad_config_t vad_config;
    memset(&vad_config, 0, sizeof(ty_vad_config_t));
    vad_config.start_threshold_ms = 300;
    vad_config.end_threshold_ms = 500;
    vad_config.silence_threshold_ms = 0;
    vad_config.sample_rate = TKL_AUDIO_SAMPLE_16K;
    vad_config.channel = TKL_AUDIO_CHANNEL_MONO;
    vad_config.vad_frame_duration = 10;
    vad_config.scale = 2.5;
    TUYA_CALL_ERR_RETURN(ty_vad_app_init(&vad_config));
    TUYA_CALL_ERR_RETURN(ty_vad_app_start());

    return OPRT_OK;
}
#endif

static OPERATE_RET __ai_audio_open_asr(void)
{
    OPERATE_RET rt = OPRT_OK;

    TUYA_CALL_ERR_RETURN(tkl_asr_init());
    TUYA_CALL_ERR_RETURN(tkl_asr_wakeup_word_config(cWAKEUP_KEYWORD_LIST, CNTSOF(cWAKEUP_KEYWORD_LIST)));

    sg_audio_input.asr_buff_len = tkl_asr_get_process_uint_size() * 10;
    sg_audio_input.asr_buff = tkl_system_psram_malloc(sg_audio_input.asr_buff_len);
    if (NULL == sg_audio_input.asr_buff) {
        PR_ERR("malloc failed");
        return OPRT_MALLOC_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET ai_audio_input_init(AI_AUDIO_INPUT_CFG_T *cfg, AI_AUDIO_INOUT_INFORM_CB cb)
{
    OPERATE_RET rt = OPRT_OK;

    if (NULL == cfg || NULL == cb) {
        return OPRT_INVALID_PARM;
    }

    if (true == sg_audio_input.is_init) {
        return OPRT_OK;
    }

    TUYA_CALL_ERR_RETURN(
        tuya_ring_buff_create(INPUT_RINGBUFF_LEN, OVERFLOW_PSRAM_STOP_TYPE, &sg_audio_input.ringbuff_hdl));
    TUYA_CALL_ERR_RETURN(tal_mutex_create_init(&sg_audio_input.rb_mutex));

    TUYA_CALL_ERR_RETURN(__ai_audio_input_hardware_init());

    if (cfg->is_open_vad) {
        TUYA_CALL_ERR_RETURN(__ai_audio_open_vad());
        sg_audio_input.is_open_vad = true;
    }

    if (cfg->is_open_asr) {
        TUYA_CALL_ERR_RETURN(__ai_audio_open_asr());
        sg_audio_input.is_open_asr = true;
    }
    sg_audio_input.is_enable_interrupt = cfg->is_enable_interrupt;

    TUYA_CALL_ERR_RETURN(tal_sw_timer_create(__ai_audio_input_enter_idle, NULL, &sg_audio_input.wakeup_timer_id));

    sg_audio_input.wakeup_timeout_ms = cfg->wakeup_timeout_ms;
    sg_audio_input_inform_cb = cb;

    TUYA_CALL_ERR_RETURN(tkl_thread_create_in_psram(&sg_ai_audio_input_thrd_hdl, "audio_input", 1024 * 4, THREAD_PRIO_1,
                                                    __ai_audio_handle_frame_task, NULL));

    return OPRT_OK;
}

OPERATE_RET ai_audio_input_start(void)
{
    if (sg_audio_input.is_open_vad) {
        sg_audio_input.state = AI_AUDIO_INPUT_STATE_DETECTING;
    } else {
        sg_audio_input.state = AI_AUDIO_INPUT_STATE_AWAKE;
    }

    return OPRT_OK;
}

OPERATE_RET ai_audio_input_stop(void)
{
    if (sg_audio_input.is_open_vad) {
        sg_audio_input.state = AI_AUDIO_INPUT_STATE_DETECTING;
    } else {
        sg_audio_input.state = AI_AUDIO_INPUT_STATE_IDLE;
    }
}

AI_AUDIO_INPUT_STATE_E ai_audio_input_get_state(void)
{
    return sg_audio_input.state;
}