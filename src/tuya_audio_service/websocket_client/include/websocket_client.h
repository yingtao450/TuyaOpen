/**
 * @file websocket_client.h
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

#ifndef __WEBSOCKET_CLIENT_H__
#define __WEBSOCKET_CLIENT_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WS_HANDSHAKE_CONN_TIMEOUT       (10)//s
#define WS_HANDSHAKE_RECV_TIMEOUT       (2*1000)//ms
#define WS_RECONNECT_WAIT_TIME          (2*1000)//(5*1000)//ms

typedef uint8_t WS_CONN_STATE_T;
#define WS_CONN_STATE_NONE              (0)
#define WS_CONN_STATE_FAILED            (1)
#define WS_CONN_STATE_SUCCESS           (2)

typedef struct {
    char *uri;                            //<ws/wss>://host[:port]/path
    uint32_t handshake_conn_timeout;      // Handshake connection phase: TLS connection timeout, in seconds
    uint32_t handshake_recv_timeout;      // Handshake receiving phase: socket receive timeout, in milliseconds
    uint32_t reconnect_wait_time;         // Wait time for reconnection after connection failure, in milliseconds
    uint32_t keep_alive_time;             // Keep-alive interval, in milliseconds
    void (*recv_bin_cb)(uint8_t *data, size_t len);
    void (*recv_text_cb)(uint8_t *data, size_t len);
} WEBSOCKET_CLIENT_CFG_S;

typedef void *WEBSOCKET_HANDLE_T;

/**
 * @brief Create a new WebSocket client instance
 * 
 * @param[out] pp_handle Pointer to store the created WebSocket handle
 * @param[in] cfg WebSocket client configuration parameters
 * @return OPERATE_RET 
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_create(WEBSOCKET_HANDLE_T *pp_handle, WEBSOCKET_CLIENT_CFG_S *cfg);

/**
 * @brief Destroy the WebSocket client instance and free resources
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_destory(WEBSOCKET_HANDLE_T handle);

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
OPERATE_RET websocket_client_start(WEBSOCKET_HANDLE_T handle);

/**
 * @brief Connect the WebSocket client to the server
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_connect(WEBSOCKET_HANDLE_T handle);

/**
 * @brief Disconnect the WebSocket client
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_disconnect(WEBSOCKET_HANDLE_T handle);

/**
 * @brief Shutdown the WebSocket client
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_shutdown(WEBSOCKET_HANDLE_T handle);

/**
 * @brief Receive data from the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_receive(WEBSOCKET_HANDLE_T handle);

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
OPERATE_RET websocket_client_send_text(WEBSOCKET_HANDLE_T handle, uint8_t *data, uint32_t len);

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
OPERATE_RET websocket_client_send_bin(WEBSOCKET_HANDLE_T handle, uint8_t *data, uint32_t len);

/**
 * @brief Send a ping frame through the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_send_ping(WEBSOCKET_HANDLE_T handle);

/**
 * @brief Send a pong frame through the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_send_pong(WEBSOCKET_HANDLE_T handle);

/**
 * @brief Send a close frame through the WebSocket connection
 * 
 * @param[in] handle WebSocket client handle
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_send_close(WEBSOCKET_HANDLE_T handle);

/**
 * @brief Get the current connection status of the WebSocket client
 * 
 * @param[in] handle WebSocket client handle
 * @param[out] status Pointer to store the connection status
 * @return OPERATE_RET
 *         - OPRT_OK: Success
 *         - Others: Failure
 */
OPERATE_RET websocket_client_get_conn_status(WEBSOCKET_HANDLE_T handle, WS_CONN_STATE_T *status);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /** !__WEBSOCKET_CLIENT_H__ */
