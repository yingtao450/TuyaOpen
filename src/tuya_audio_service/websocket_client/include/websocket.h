/**
 * @file websocket.h
 * @brief Defines the core WebSocket protocol interface and data structures
 *
 * This header file provides the fundamental definitions and structures for
 * WebSocket protocol implementation. It includes enums for WebSocket status
 * codes, state machine management, thread states, and the main WebSocket
 * handle structure. The implementation supports connection management,
 * heartbeat mechanisms, TLS encryption, and callback functions for binary
 * and text message handling. This file is essential for developers working
 * on IoT applications that require reliable bidirectional communication
 * through the WebSocket protocol with proper state management and security
 * features.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include "tuya_cloud_types.h"
#include "tal_api.h"
#include "tuya_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WebSocket status code
 **/
typedef enum {
    WS_STATUS_CODE_NORMAL_CLOSURE               = 1000,
    WS_STATUS_CODE_GOING_AWAY                   = 1001,
    WS_STATUS_CODE_PROTOCOL_ERROR               = 1002,
    WS_STATUS_CODE_UNSUPPORTED_DATA             = 1003,
    WS_STATUS_CODE_NO_STATUS_RCVD               = 1005,
    WS_STATUS_CODE_ABNORMAL_CLOSURE             = 1006,
    WS_STATUS_CODE_INVALID_PAYLOAD_DATA         = 1007,
    WS_STATUS_CODE_POLICY_VIOLATION             = 1008,
    WS_STATUS_CODE_MESSAGE_TOO_BIG              = 1009,
    WS_STATUS_CODE_MANDATORY_EXT                = 1010,
    WS_STATUS_CODE_INTERNAL_ERROR               = 1011,
    WS_STATUS_CODE_TLS_HANDSHAKE                = 1015
} WEBSOCKET_STATUS_CODE_E;

/**
 * @brief WebSocket state machine management
 **/
typedef enum {
    WS_RUN_STATE_UNUSED                         = 0,
    WS_RUN_STATE_INIT                           = 1,
    WS_RUN_STATE_CONNECT                        = 2,
    WS_RUN_STATE_RECEIVE                        = 3,
    WS_RUN_STATE_WAIT                           = 4,
    WS_RUN_STATE_SHUTDOWN                       = 5,
} WEBSOCKET_RUN_STATE_E;

/**
 * @brief WebSocket thread run state
 **/
typedef enum {
    WS_THRD_STATE_INIT                          = 0,
    WS_THRD_STATE_RUNNING                       = 1,
    WS_THRD_STATE_QUIT_CMD                      = 2,
    WS_THRD_STATE_RELEASE                       = 3,
} WEBSOCKET_THRD_STATE_E;

typedef struct {
    TIMER_ID    tm_id; //timer message info
    TIME_MS     timeout;
} WEBSOCKET_HEARTBEAT_S;

typedef struct __WEBSOCKET_H__{
    char *uri;
    char *path;
    char *origin; //example, "http://coolaf.com\r\n"
    char *sub_prot;
    char *host;
    TUYA_IP_ADDR_T hostaddr;
    uint16_t port;
    int sockfd;
    BOOL_T tls_enable;
    tuya_tls_hander tls_hander;

    uint32_t handshake_conn_timeout;
    uint32_t handshake_recv_timeout; //ms
    uint32_t reconnect_wait_time;
    uint32_t fail_cnt;
    BOOL_T is_connected;

    WEBSOCKET_THRD_STATE_E thrd_state;
    THREAD_HANDLE thrd_handle;
    WEBSOCKET_RUN_STATE_E run_state;
    MUTEX_HANDLE          mutex;

    SEM_HANDLE sem_link;
    uint32_t keep_alive_time;

    WEBSOCKET_HEARTBEAT_S hb_ping;
    WEBSOCKET_HEARTBEAT_S hb_pong;
    uint32_t                ping_count;
    uint32_t                pong_count;

    void (*recv_bin_cb)(uint8_t *data, size_t len);
    void (*recv_text_cb)(uint8_t *data, size_t len);


} WEBSOCKET_S;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /** !__WEBSOCKET_H__ */
