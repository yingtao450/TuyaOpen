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
// PCM frame size: 320 bytes
#define AI_AUDIO_PCM_FRAME_TM_MS (10)
#define AI_AUDIO_PCM_FRAME_SIZE  (320)
#define AI_AUDIO_VAD_ACITVE_TM_MS (600)    // vad active duration

#define AI_AUDIO_VOICE_FRAME_LEN_GET(tm_ms) ((tm_ms) / AI_AUDIO_PCM_FRAME_TM_MS * AI_AUDIO_PCM_FRAME_SIZE)
/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    AI_AUDIO_INPUT_STATE_IDLE,
    AI_AUDIO_INPUT_STATE_DETECTING,     // detect vad and wakeup keyword
    AI_AUDIO_INPUT_STATE_DETECTED_WORD, // detected wakeup word
    AI_AUDIO_INPUT_STATE_AWAKE,
} AI_AUDIO_INPUT_STATE_E;

typedef enum {
    AI_AUDIO_INPUT_EVT_NONE,
    AI_AUDIO_INPUT_EVT_IDLE,
    AI_AUDIO_INPUT_EVT_ENTER_DETECT, // detecting vad and wakeup keywor
    AI_AUDIO_INPUT_EVT_DETECTING,
    AI_AUDIO_INPUT_EVT_ASR_WORD,
    AI_AUDIO_INPUT_EVT_WAKEUP,
    AI_AUDIO_INPUT_EVT_AWAKE,
} AI_AUDIO_INPUT_EVENT_E;

typedef enum {
    AI_AUDIO_INPUT_WAKEUP_MANUAL, // manual trigger wakeup
    AI_AUDIO_INPUT_WAKEUP_VAD,    // detect vad
    AI_AUDIO_INPUT_WAKEUP_ASR,    // detect vad and asr wakeup keyword
    AI_AUDIO_INPUT_WAKEUP_MAX,
} AI_AUDIO_INPUT_WAKEUP_TP_E;

typedef struct {
    AI_AUDIO_INPUT_WAKEUP_TP_E wakeup_tp;
} AI_AUDIO_INPUT_CFG_T;

typedef void (*AI_AUDIO_INOUT_INFORM_CB)(AI_AUDIO_INPUT_EVENT_E event, uint8_t *data, uint32_t len, void *arg);

/***********************************************************
********************function declaration********************
***********************************************************/

OPERATE_RET ai_audio_input_init(AI_AUDIO_INPUT_CFG_T *cfg, AI_AUDIO_INOUT_INFORM_CB cb);

OPERATE_RET ai_audio_input_open(void);

OPERATE_RET ai_audio_input_close(void);

OPERATE_RET ai_audio_input_restart_asr_detect_wakeup_word(void);

OPERATE_RET ai_audio_input_trigger_asr_awake(void);

OPERATE_RET ai_audio_input_set_wakeup_tp(AI_AUDIO_INPUT_WAKEUP_TP_E wakeup_tp);

#ifdef __cplusplus
}
#endif

#endif /* __AI_AUDIO_INPUT_H__ */
