/**
 * @file tuya_speaker_voice_gw.h
 * @brief voice stream gateway access
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */
#ifndef __TY_SPEAKER_VOICE_GW_H__
#define __TY_SPEAKER_VOICE_GW_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TUYA_SPEAKER_WS_CB)(uint8_t *data, size_t len);

OPERATE_RET tuya_speaker_ws_client_init(TUYA_SPEAKER_WS_CB bin_cb, TUYA_SPEAKER_WS_CB text_cb);

OPERATE_RET tuya_speaker_ws_client_start(void);
OPERATE_RET tuya_speaker_ws_client_stop(void);

OPERATE_RET tuya_speaker_ws_send_bin(uint8_t *data, uint32_t len);
OPERATE_RET tuya_speaker_ws_send_text(uint8_t *data, uint32_t len);

BOOL_T tuya_speaker_ws_is_online(void);

OPERATE_RET tuya_speaker_del_domain_name(void);

void tuya_speaker_ws_disconnect(void);

void tuya_speaker_ws_set_keepalive(uint32_t sec);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
