/**
 * @file ai_audio.h
 * @version 0.1
 * @date 2025-04-15
 */

#include "tuya_cloud_types.h"
#include "ai_audio_agent.h"
#include "ai_audio_cloud_asr.h"
#include "ai_audio_player.h"
#include "ai_audio_input.h"
/***********************************************************
************************macro define************************
***********************************************************/
typedef uint8_t  AI_AUDIO_WORK_MODE_E;
#define AI_AUDIO_WORK_MODE_HOLD         1
#define AI_AUDIO_WORK_MODE_TRIGGER      2
#define AI_AUDIO_WORK_MODE_WAKEUP       3
#define AI_AUDIO_WORK_MODE_FREE         4

/***********************************************************
***********************typedef define***********************
***********************************************************/

typedef struct {
    AI_AUDIO_WORK_MODE_E    work_mode;
    AI_AGENT_MSG_CB         agent_msg_cb;
} AI_AUDIO_CONFIG_T;

/***********************************************************
********************function declaration********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define**********************
***********************************************************/
OPERATE_RET ai_audio_init(AI_AUDIO_CONFIG_T *cfg);

OPERATE_RET ai_audio_set_volume(uint8_t volume);

uint8_t ai_audio_get_volume(void);

OPERATE_RET ai_audio_set_open(bool is_open);

OPERATE_RET ai_audio_set_work_mode(AI_AUDIO_WORK_MODE_E work_mode);