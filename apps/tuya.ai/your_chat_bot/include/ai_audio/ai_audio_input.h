/**
 * @file ai_audio_input.h
 * @version 0.1
 * @date 2025-04-17
 */

#ifndef __AI_AUDIO_INPUT_H__
#define __AI_AUDIO_INPUT_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    AI_AUDIO_INPUT_STATE_IDLE,
    AI_AUDIO_INPUT_STATE_DETECTING, // detect vad and wakeup keyword
    AI_AUDIO_INPUT_STATE_AWAKE,
} AI_AUDIO_INPUT_STATE_E;

typedef enum {
    AI_AUDIO_INPUT_EVT_IDLE,
    AI_AUDIO_INPUT_EVT_ENTER_DETECT, // detecting vad and wakeup keywor
    AI_AUDIO_INPUT_EVT_DETECTING,
    AI_AUDIO_INPUT_EVT_WAKEUP,
    AI_AUDIO_INPUT_EVT_ASR_WAKEUP,
    AI_AUDIO_INPUT_EVT_AWAKE,
} AI_AUDIO_INPUT_EVENT_E;

typedef struct {
    uint8_t is_open_vad;
    uint8_t is_open_asr;
    uint8_t is_enable_interrupt;
    uint8_t wakeup_timeout_ms;
} AI_AUDIO_INPUT_CFG_T;

typedef void (*AI_AUDIO_INOUT_INFORM_CB)(AI_AUDIO_INPUT_EVENT_E state, uint8_t *data, uint32_t len, void *arg);

/***********************************************************
********************function declaration********************
***********************************************************/

OPERATE_RET ai_audio_input_init(AI_AUDIO_INPUT_CFG_T *cfg, AI_AUDIO_INOUT_INFORM_CB cb);

OPERATE_RET ai_audio_input_start(void);

OPERATE_RET ai_audio_input_stop(void);

OPERATE_RET ai_audio_input_restart(void);

AI_AUDIO_INPUT_STATE_E ai_audio_input_get_state(void);

OPERATE_RET ai_audio_input_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __AI_AUDIO_INPUT_H__ */
