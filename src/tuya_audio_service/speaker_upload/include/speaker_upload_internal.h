/**
 * @file speaker_upload_internal.h
 * @brief Defines internal structures and interfaces for Tuya's speaker upload service.
 *
 * This source file provides the internal implementation details and data structures
 * for the speaker audio upload functionality within the Tuya audio service framework.
 * It includes the task management, state tracking, and encoding configurations for
 * audio upload operations, which handle the internal processing and management of
 * speaker audio data. The implementation supports various internal mechanisms such
 * as timer management, mutex protection, and task state monitoring to ensure robust
 * audio upload operations. This file is essential for developers maintaining and
 * extending the core speaker upload functionality in Tuya's IoT audio framework.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef SPEAKER_UPLOAD_INTERNAL_H
#define SPEAKER_UPLOAD_INTERNAL_H

#include "speaker_upload.h"

#include "tal_mutex.h"
#include "tal_sw_timer.h"

#ifdef __cplusplus
extern "C" {
#endif



#define TY_SPEAKER_UP_MGR_TIMER_INTR    1000    ///< unit ms
#define TY_UPLOAD_CHECK_TASK_INTR       5
#define TY_UPLOAD_TASK_TIMEOUT          10

typedef enum {
    UPLOAD_TASK_STAT_INIT   = 0,
    UPLOAD_TASK_START       = 1,
    UPLOAD_TASK_ERR         = 2,
    UPLOAD_TASK_NET_ERR     = 3,
    UPLOAD_TASK_END         = 4,
} UPLOAD_TASK_STAT_E;

typedef struct {
    TIME_T  start_tm;
    BOOL_T  net_alarm_flag;
    TIME_T  last_upload_tm;
} UPLOAD_TASK_STAT_MANAGE_S;

typedef struct {
    UPLOAD_TASK_STAT_E              upload_stat;
    UPLOAD_TASK_STAT_MANAGE_S       stat_manage;
    SPEAKER_MEDIA_ENCODER_S         encoder;
    TUYA_VOICE_UPLOAD_T             ctx;
    SPEAKER_UPLOAD_REPORT_STAT_CB   report_stat_cb;
    TIMER_ID                        mgr_tm;
} MEDIA_UPLOAD_TASK_S;

typedef struct {
    MEDIA_UPLOAD_TASK_S             task;
    TIMER_ID                        mgr_tm;
    SPEAKER_UPLOAD_CONFIG_S         config;
    MUTEX_HANDLE                    mutex;
    BOOL_T                          is_uploading;
    BOOL_T                          is_encoding;
} MEDIA_UPLOAD_MGR_S;

#ifdef __cplusplus
}
#endif

#endif /** !SPEAKER_UPLOAD_INTERNAL_H */
