/**
 * @file websocket_client.c
 * @brief Implements WebSocket client functionality for network communication
 *
 * This source file provides the implementation of a WebSocket client that handles
 * bi-directional communication over WebSocket protocol. It includes functionality
 * for connection management, message framing, heartbeat mechanisms, and data
 * transmission. The implementation supports both text and binary message formats,
 * ping/pong for connection health monitoring, and automatic reconnection on
 * failures. This file is essential for developers working on IoT applications
 * that require reliable real-time communication over WebSocket protocol.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#include "websocket_utils.h"
#include "websocket_client.h"
#include "websocket_frame.h"
#include "websocket_conn.h"
#include "websocket_netio.h"
#include "websocket_timer.h"
#include "netmgr.h"
#include "tal_api.h"

#ifndef VOICE_PROTOCOL_STREAM_GW_KEEP_ALIVE_TIME
#define WS_HB_PING_TIME_INTERVAL            (5 * 1000)  /**< unit millisecond */
#define WS_HB_PONG_TIMEOUT                  (16 * 1000) /**< unit millisecond */
#else
#define WS_HB_PING_TIME_INTERVAL            (VOICE_PROTOCOL_STREAM_GW_KEEP_ALIVE_TIME/3 * 1000)  /**< unit millisecond */
#define WS_HB_PONG_TIMEOUT                  ((VOICE_PROTOCOL_STREAM_GW_KEEP_ALIVE_TIME+1) * 1000) /**< unit millisecond */
#endif
static WEBSOCKET_S *s_websocket_clinet = NULL;
static BOOL_T s_net_link_up_connected = TRUE;
static void __set_handshake_conn_timeout(WEBSOCKET_S *ws, uint32_t timeout_s)
{
    ws->handshake_conn_timeout = (timeout_s <= 0) ? WS_HANDSHAKE_CONN_TIMEOUT : timeout_s;
    PR_DEBUG("websocket %p handshake conn max time(s): %u", ws, ws->handshake_conn_timeout);
}

static void __set_handshake_recv_timeout(WEBSOCKET_S *ws, uint32_t timeout_ms)
{
    ws->handshake_recv_timeout = (timeout_ms <= 0) ? WS_HANDSHAKE_RECV_TIMEOUT : timeout_ms;
    PR_DEBUG("websocket %p handshake recv max time(ms): %u", ws, ws->handshake_recv_timeout);
}

static void __set_reconnect_wait_time(WEBSOCKET_S *ws, uint32_t wait_ms)
{
    ws->reconnect_wait_time = (wait_ms <= 0) ? WS_RECONNECT_WAIT_TIME : wait_ms;
    PR_DEBUG("websocket %p reconnect max wait time(ms): %u", ws, ws->reconnect_wait_time);
}

static OPERATE_RET __enable_disconnect(WEBSOCKET_S *ws, char *tag)
{
    if (tag != NULL)
        PR_DEBUG("websocket %p disconnect TAG: %s", ws, tag);

    if (ws != NULL && ws->is_connected) {
        ws->is_connected = FALSE;
        websocket_hb_pong_timer_stop(ws);
        websocket_hb_ping_timer_stop(ws);
        websocket_netio_close(ws);
        PR_WARN("websocket %p disconnect enabled, now is %s.", ws, ws->is_connected ? "connected" : "disconnected");
    }

    PR_WARN("websocket %p already disconnected", ws);

    return OPRT_OK;
}

static OPERATE_RET __websocket_disconnect(WEBSOCKET_HANDLE_T handle, char *tag)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(ws->mutex);

    PR_DEBUG("websocket %p client disconnect, tag: %s, run_state: %d, thrd_state: %d",
             ws, tag, ws->run_state, ws->thrd_state);

    WS_ASSERT(OPRT_OK == tal_mutex_lock(ws->mutex));
    __enable_disconnect(ws, tag);
    WS_ASSERT(OPRT_OK == tal_mutex_unlock(ws->mutex));

    return OPRT_OK;
}

static OPERATE_RET __websocket_shutdown(WEBSOCKET_HANDLE_T handle, char *tag)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(ws->mutex);

    PR_DEBUG("websocket %p client shutdown, tag: %s, run_state: %d, thrd_state: %d",
             ws, tag, ws->run_state, ws->thrd_state);

    WS_ASSERT(OPRT_OK == tal_mutex_lock(ws->mutex));
    if (ws->thrd_state == WS_THRD_STATE_RUNNING) {
        __enable_disconnect(ws, tag);
        ws->run_state = WS_RUN_STATE_CONNECT;
        PR_WARN("websocket %p client shutdown successful", ws);
    }
    WS_ASSERT(OPRT_OK == tal_mutex_unlock(ws->mutex));

    if (ws->thrd_state == WS_THRD_STATE_RUNNING && s_net_link_up_connected) {
        PR_WARN("websocket %p client reconnect time: %u", ws, ws->reconnect_wait_time);
        tal_system_sleep(ws->reconnect_wait_time);
    }

    return OPRT_OK;
}

static OPERATE_RET __websocket_destory(WEBSOCKET_HANDLE_T handle, char *tag)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(ws->mutex);

    PR_DEBUG("websocket %p client destory, tag: %s, run_state: %d, thrd_state: %d",
             ws, tag, ws->run_state, ws->thrd_state);

    WS_ASSERT(OPRT_OK == tal_mutex_lock(ws->mutex));
    ws->thrd_state = WS_THRD_STATE_QUIT_CMD;
    ws->run_state = WS_RUN_STATE_SHUTDOWN;
    __enable_disconnect(ws, tag);
    websocket_hb_timer_release(ws);
    WS_ASSERT(OPRT_OK == tal_mutex_unlock(ws->mutex));

    if (ws->thrd_state == WS_THRD_STATE_RUNNING) {
        while (ws->thrd_state != WS_THRD_STATE_RELEASE) {
            tal_system_sleep(1);
        }
    }

    WS_ASSERT(OPRT_OK == tal_mutex_release(ws->mutex));
    WS_ASSERT(OPRT_OK == tal_mutex_release(ws->sem_link));
    WS_SAFE_FREE(ws->uri);
    WS_SAFE_FREE(ws->path);
    WS_SAFE_FREE(ws->origin);
    WS_SAFE_FREE(ws->sub_prot);
    WS_SAFE_FREE(ws->host);
    WS_SAFE_FREE(ws);
    PR_DEBUG("websocket client destory successful");

    return OPRT_OK;
}

static void __websocket_hb_ping_cb(TIMER_ID timer_id, void *arg)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)arg;
    OPERATE_RET rt = OPRT_OK;

#define WEBSOCKET_PING_DEBUG_COUNT  12
    if (ws->ping_count >= WEBSOCKET_PING_DEBUG_COUNT) {
        WS_DEBUG("websocket %p send ping count: %d, pong count: %d <-->",
                ws, ws->ping_count, ws->pong_count);
        ws->ping_count = 0;
        ws->pong_count = 0;
    }
    if (ws->is_connected) {
        ws->ping_count ++;
        rt = websocket_client_send_ping(ws);
        if (OPRT_OK != rt) {
            PR_ERR("websocket %p ping failed, ready to close", ws);
            __websocket_disconnect(ws, "WS_PING_SEND_ERR");
            return ;
        }

        if (!tal_sw_timer_is_running(ws->hb_pong.tm_id)) {
            websocket_hb_pong_timer_start(ws);
            WS_DEBUG("websocket open pong timer");
        }
    }
}

static void __websocket_hb_pong_cb(TIMER_ID timer_id, void *arg)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)arg;

    if (ws->is_connected) {
        PR_ERR("websocket %p pong timeout", ws);
        __websocket_disconnect(ws, "WS_PONG_RECV_TIMEOUT");
    }
}

static OPERATE_RET __websocket_update_wait_reconnect(WEBSOCKET_S *ws)
{
    WS_CHECK_NULL_RET(ws);

    ws->fail_cnt++;

    int sleeptime = 1000 + tal_system_get_random(ws->reconnect_wait_time + ws->fail_cnt * 1000);
    if (sleeptime > ws->reconnect_wait_time)
        sleeptime = ws->reconnect_wait_time;

    PR_DEBUG("websocket %p fail_cnt: %u, wait reconnect sleeptime: %d ms", ws, ws->fail_cnt, sleeptime);
    tal_system_sleep((TIME_MS)sleeptime);

    return OPRT_OK;
}

static void __websocket_frame_recv_cb(WEBSOCKET_S *ws, WEBSOCKET_FRAME_TYPE_E type, BOOL_T final, void *data, size_t len)
{
    if (tal_sw_timer_is_running(ws->hb_pong.tm_id)) {
        websocket_hb_pong_timer_stop(ws);
    }

    switch (type) {
    case WS_FRAME_TYPE_PING:
        PR_WARN("websocket %p recv ping <--", ws);
        websocket_client_send_pong(ws);
        break;

    case WS_FRAME_TYPE_PONG:
        ws->pong_count++;
        ws->fail_cnt = 0;
        // websocket_hb_pong_timer_stop(ws);
        break;

    case WS_FRAME_TYPE_TEXT:
        WS_DEBUG("websocket %p recv text", ws);
        if (ws->recv_text_cb)
            ws->recv_text_cb((uint8_t *)data, len);
        break;

    case WS_FRAME_TYPE_BINARY:
        WS_DEBUG("websocket %p recv binary", ws);
        if (ws->recv_bin_cb)
            ws->recv_bin_cb((uint8_t *)data, len);
        break;
        
    default :
        PR_ERR("websocket %p recv invalid type: %d", ws, type);
        break;
    }
}

static OPERATE_RET __check_cfg_para_validity(WEBSOCKET_CLIENT_CFG_S *cfg)
{
    WS_CHECK_NULL_RET(cfg);
    WS_CHECK_NULL_RET(cfg->uri);
    WS_CHECK_NULL_RET(cfg->recv_bin_cb);
    return OPRT_OK;
}

static char *__get_tid(char *uri)
{
    char *p_tid = strstr(uri, "tid=");
    return ((p_tid != NULL) ? (p_tid + strlen("tid=")) : "");
}

/**
 * @brief dump websocket client object instance
 */
#if defined(NDEBUG)
#define DUMP_WEBSOCKET(ws)  (void *)0
#else
#define DUMP_WEBSOCKET(ws)                                                      \
{                                                                               \
    if (ws) {                                                                   \
        PR_DEBUG("websocket %p:", ws);                                          \
        PR_DEBUG("\turi: %s", ws->uri);                                         \
        PR_DEBUG("\tpath: %s", ws->path);                                       \
        PR_DEBUG("\torigin: %s", ws->origin);                                   \
        PR_DEBUG("\tsub_prot: %s", ws->sub_prot);                               \
        PR_DEBUG("\thost: %s", ws->host);                                       \
        PR_DEBUG("\tport: %d", ws->port);                                       \
        PR_DEBUG("\tsockfd: %d", ws->sockfd);                                   \
        PR_DEBUG("\ttls_enable: %d", ws->tls_enable);                           \
        PR_DEBUG("\ttls_hander: %p", ws->tls_hander);                           \
        PR_DEBUG("\thandshake_conn_timeout: %u", ws->handshake_conn_timeout);   \
        PR_DEBUG("\thandshake_recv_timeout: %u", ws->handshake_recv_timeout);   \
        PR_DEBUG("\reconnect_wait_time: %u", ws->reconnect_wait_time);          \
        PR_DEBUG("\tfail_cnt: %u", ws->fail_cnt);                               \
        PR_DEBUG("\tis_connected: %d", ws->is_connected);                       \
        PR_DEBUG("\tthrd_state: %d", ws->thrd_state);                           \
        PR_DEBUG("\tthrd_handle: %p", ws->thrd_handle);                         \
        PR_DEBUG("\trun_state: %d", ws->run_state);                             \
        PR_DEBUG("\tmutex: %p", ws->mutex);                                     \
        PR_DEBUG("\trecv_text_cb: %p", ws->recv_text_cb);                       \
        PR_DEBUG("\trecv_bin_cb: %p", ws->recv_bin_cb);                         \
    }                                                                           \
}
#endif


static int __wsc_event_linkup_cb(void *data)
{
    PR_DEBUG("__wsc_event_linkup_cb");
    if (s_websocket_clinet) {
        WS_ASSERT(OPRT_OK == tal_mutex_lock(s_websocket_clinet->mutex));
        s_net_link_up_connected = TRUE;
        WS_ASSERT(OPRT_OK == tal_mutex_unlock(s_websocket_clinet->mutex));
        tal_semaphore_post(s_websocket_clinet->sem_link);
    }

    return OPRT_OK;
}

static int __wsc_event_linkdown_cb(void *data)
{
    PR_DEBUG("__wsc_event_linkdown_cb");
    if (s_websocket_clinet) {
        WS_ASSERT(OPRT_OK == tal_mutex_lock(s_websocket_clinet->mutex));
        if (s_websocket_clinet->run_state != WS_RUN_STATE_SHUTDOWN){
            s_websocket_clinet->run_state = WS_RUN_STATE_SHUTDOWN;
            s_net_link_up_connected = FALSE;
        }
        WS_ASSERT(OPRT_OK == tal_mutex_unlock(s_websocket_clinet->mutex));
    }


    return OPRT_OK;
}

static int __wsc_event_linkstatus_cb(void *data)
{
    netmgr_status_e status = (netmgr_status_e)data;
    if (NETMGR_LINK_DOWN == status) {
        __wsc_event_linkdown_cb(data);
    } else if (NETMGR_LINK_UP == status){
        __wsc_event_linkup_cb(data);
    }

    return OPRT_OK;
}


/**
 * @brief Create a new WebSocket client instance
 * 
 * @param[out] pp_handle Pointer to store the created WebSocket handle
 * @param[in] cfg WebSocket client configuration parameters
 * @return OPERATE_RET 
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_create(WEBSOCKET_HANDLE_T *pp_handle, WEBSOCKET_CLIENT_CFG_S *cfg)
{
    WEBSOCKET_S *ws = NULL;
    OPERATE_RET rt = OPRT_OK;

    WS_CALL_ERR_RET(__check_cfg_para_validity(cfg));

    WS_MALLOC_ZERO_ERR_GOTO(ws, sizeof(WEBSOCKET_S));
    ws->run_state = WS_RUN_STATE_INIT;
    WS_CALL_ERR_GOTO(tal_mutex_create_init(&ws->mutex));
    WS_CALL_ERR_GOTO(websocket_handshake_init(ws, cfg->uri));
    __set_handshake_conn_timeout(ws, cfg->handshake_conn_timeout);
    __set_handshake_recv_timeout(ws, cfg->handshake_recv_timeout);
    __set_reconnect_wait_time(ws, cfg->reconnect_wait_time);

    websocket_hb_timer_create(ws, __websocket_hb_ping_cb, __websocket_hb_pong_cb);

    if  (cfg->keep_alive_time && cfg->keep_alive_time > WS_HB_PING_TIME_INTERVAL) {
        // Send ping heartbeat at 85% of the keep-alive time
        websocket_hb_ping_timer_init(ws,(TIME_MS)((cfg->keep_alive_time*17) /20));
        // Interval between two heartbeats
        websocket_hb_pong_timer_init(ws, cfg->keep_alive_time*2);
        ws->keep_alive_time = cfg->keep_alive_time;
    }
    else {
        websocket_hb_ping_timer_init(ws, WS_HB_PING_TIME_INTERVAL);
        websocket_hb_pong_timer_init(ws, WS_HB_PONG_TIMEOUT);
    }


    if (cfg->recv_bin_cb)
        ws->recv_bin_cb = cfg->recv_bin_cb;
    if (cfg->recv_text_cb)
        ws->recv_text_cb = cfg->recv_text_cb;

    // DUMP_WEBSOCKET(ws);
    s_websocket_clinet = ws;
    *pp_handle = (WEBSOCKET_HANDLE_T)ws;
    PR_DEBUG("websocket %p create successful, default run_state: %d, thrd_state: %d",
             ws, ws->run_state, ws->thrd_state);

    WS_CALL_ERR_GOTO(tal_semaphore_create_init(&ws->sem_link, 0, 1));
    tal_event_subscribe(EVENT_LINK_STATUS_CHG, "wsc_app", __wsc_event_linkstatus_cb, 0);
    // tal_event_subscribe(EVENT_LINK_DOWN, "wsc_app", __wsc_event_linkdown_cb, 0);
    return OPRT_OK;

err_exit:
    __websocket_destory(ws, "WS_CREATE_FAILED");
    ws = NULL;
    return rt;
}

/**
 * @brief Connect the WebSocket client to the server
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_connect(WEBSOCKET_HANDLE_T handle)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    OPERATE_RET rt = OPRT_OK;
    WS_CHECK_NULL_RET(ws);

    PR_DEBUG("websocket %p connect", ws);

    WS_CALL_ERR_RET(websocket_handshake_start(ws));
    WS_CALL_ERR_RET(websocket_hb_ping_timer_start(ws));

    return OPRT_OK;
}

/**
 * @brief Receive data from the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_receive(WEBSOCKET_HANDLE_T handle)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    WS_CHECK_NULL_RET(ws);

    WS_DEBUG("websocket %p receive", ws);

    return websocket_recv_frame(ws, __websocket_frame_recv_cb);;
}

/**
 * @brief Send text data through the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @param[in] data Text data to send
 * @param[in] len Length of the data
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_send_text(WEBSOCKET_HANDLE_T handle, uint8_t *data, uint32_t len)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    return websocket_send_frame(ws, WS_FRAME_TYPE_TEXT, data, len, TRUE, TRUE);
}

/**
 * @brief Send binary data through the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @param[in] data Binary data to send
 * @param[in] len Length of the data
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_send_bin(WEBSOCKET_HANDLE_T handle, uint8_t *data, uint32_t len)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    return websocket_send_frame(ws, WS_FRAME_TYPE_BINARY, data, len, TRUE, TRUE);
}

/**
 * @brief Send a ping frame through the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_send_ping(WEBSOCKET_HANDLE_T handle)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    return websocket_send_frame(ws, WS_FRAME_TYPE_PING, NULL, 0, TRUE, TRUE);
}

/**
 * @brief Send a pong frame through the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_send_pong(WEBSOCKET_HANDLE_T handle)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    return websocket_send_frame(ws, WS_FRAME_TYPE_PING, NULL, 0, TRUE, TRUE);
}

/**
 * @brief Send a close frame through the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_send_close(WEBSOCKET_HANDLE_T handle)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    return websocket_send_frame(ws, WS_FRAME_TYPE_CLOSE, NULL, 0, TRUE, TRUE);
}

/**
 * @brief Disconnect the WebSocket client
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_disconnect(WEBSOCKET_HANDLE_T handle)
{
    return __websocket_disconnect(handle, NULL);
}

/**
 * @brief Shutdown the WebSocket client
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_shutdown(WEBSOCKET_HANDLE_T handle)
{
    return __websocket_shutdown(handle, NULL);
}

/**
 * @brief Destroy the WebSocket client instance and free resources
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_destory(WEBSOCKET_HANDLE_T handle)
{
    return __websocket_destory(handle, NULL);
}

/**
 * @brief Get the current connection status of the WebSocket client
 * 
 * @param[in] handle WebSocket client handle
 * @param[out] status Pointer to store the connection status
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_get_conn_status(WEBSOCKET_HANDLE_T handle, WS_CONN_STATE_T *status)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;

    WS_DEBUG("websocket %p connect get", ws);

    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(status);

    if (ws->is_connected)
        *status = WS_CONN_STATE_SUCCESS;
    else
        *status = WS_CONN_STATE_FAILED;

    WS_DEBUG("websocket %p connect status %d", ws, *status);

    return OPRT_OK;
}

static void websocket_client_work_thread(void *parameter)
{
    OPERATE_RET rt = OPRT_OK;
    WEBSOCKET_S *ws = (WEBSOCKET_S *)parameter;

    WS_ASSERT(NULL != ws);
    WS_ASSERT(NULL != ws->mutex);
    WS_ASSERT(NULL != ws->thrd_handle);

    ws->thrd_state = WS_THRD_STATE_RUNNING;

    PR_DEBUG("websocket %p thread %p start", ws, ws->thrd_handle);

    while (ws->thrd_state == WS_THRD_STATE_RUNNING) {

        WS_DEBUG("websocket %p run state: %d", ws, ws->run_state);
        if (WS_RUN_STATE_RECEIVE != ws->run_state) {
            PR_DEBUG("websocket %p run_state: %d, thrd_state: %d", ws, ws->run_state, ws->thrd_state);
        }

        switch (ws->run_state) {
        case WS_RUN_STATE_CONNECT:

            if (!s_net_link_up_connected) {
                if (tal_semaphore_wait(ws->sem_link,SEM_WAIT_FOREVER) != OPRT_OK) {
                    break;
                }
            }

            PR_INFO("websocket %p will do connect with tid: %s", ws, __get_tid(ws->uri));
            if (OPRT_OK != (rt = websocket_client_connect(ws))) {
                PR_ERR("websocket %p connect failed %d", ws, rt);
                __websocket_update_wait_reconnect(ws);
            } else {
                WS_ASSERT(OPRT_OK == tal_mutex_lock(ws->mutex));
                ws->run_state = WS_RUN_STATE_RECEIVE;
                WS_ASSERT(OPRT_OK == tal_mutex_unlock(ws->mutex));
                PR_DEBUG("websocket %p websocket_client_connect successful", ws);
            }
            break;

        case WS_RUN_STATE_RECEIVE:
            if (OPRT_OK != (rt = websocket_client_receive(ws))) {
                PR_ERR("websocket %p websocket_client_receive failed %d", ws, rt);
                WS_ASSERT(OPRT_OK == tal_mutex_lock(ws->mutex));
                ws->run_state = WS_RUN_STATE_SHUTDOWN;
                WS_ASSERT(OPRT_OK == tal_mutex_unlock(ws->mutex));
            }
            break;

        case WS_RUN_STATE_SHUTDOWN:
            if (OPRT_OK != (rt = __websocket_shutdown(ws, "WS_SHUTDOWN"))) {
                PR_ERR("websocket %p __websocket_shutdown failed %d", ws, rt);
            }
            break;

        default :
            PR_ERR("websocket %p invalid run_state: %d", ws, ws->run_state);
            break;
        }
    }

    PR_DEBUG("websocket %p thread %p stop", ws, ws->thrd_handle);
    tal_thread_delete(ws->thrd_handle);
    ws->thrd_state = WS_THRD_STATE_RELEASE;
    PR_WARN("websocket %p thread release successful, run_state: %d, thrd_state: %d",
            ws, ws->run_state, ws->thrd_state);
}

/**
 * @brief Start the WebSocket client
 * 
 * This function initializes and starts a WebSocket client thread. It performs state
 * validation and creates a worker thread for handling WebSocket communications.
 * 
 * @param[in] handle Handle to the WebSocket client instance
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Successfully started the WebSocket client
 * @retval OPRT_COM_ERROR Failed to start client or invalid run state
 * 
 * @note The client must be in WS_RUN_STATE_INIT state before starting
 * @note Thread stack size is 8KB for TLS-enabled connections, 4KB otherwise
 */
OPERATE_RET websocket_client_start(WEBSOCKET_HANDLE_T handle)
{
    WEBSOCKET_S *ws = (WEBSOCKET_S *)handle;
    int rt = OPRT_OK;
    WS_CHECK_NULL_RET(handle);

    if (ws->run_state != WS_RUN_STATE_INIT) {
        return OPRT_COM_ERROR;
    } else {
        ws->run_state = WS_RUN_STATE_CONNECT;
    }

    ws->thrd_state = WS_THRD_STATE_INIT;

    PR_DEBUG("websocket %p run_state: %d, thrd_state: %d", ws, ws->run_state, ws->thrd_state);

    THREAD_CFG_T thrd_param;
    thrd_param.thrdname   = "ws_client";
    thrd_param.stackDepth = ws->tls_enable ? 8 * 1024: 4 * 1024;
    thrd_param.priority   = THREAD_PRIO_2;

    rt = tal_thread_create_and_start(&ws->thrd_handle, NULL, NULL, websocket_client_work_thread, ws, &thrd_param);
    if (OPRT_OK != rt) {
        PR_ERR("websocket %p CreateAndStart %s failed %d", ws, thrd_param.thrdname, rt);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}
