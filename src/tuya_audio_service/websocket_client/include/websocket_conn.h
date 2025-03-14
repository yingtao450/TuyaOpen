/**
 * @file websocket_conn.h
 * @brief Implements WebSocket connection management and handshake protocol
 *
 * This source file provides the implementation of WebSocket connection handling
 * and handshake protocol functionality. It includes functions for URI parsing,
 * connection establishment, key generation and verification, and handshake
 * message formatting. The implementation supports both secure (WSS) and
 * non-secure (WS) connections, handles protocol upgrades, and manages
 * connection states. This file is essential for developers working on IoT
 * applications that require reliable WebSocket-based communication with proper
 * protocol handshaking and connection management.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __WEBSOCKET_CONN_H__
#define __WEBSOCKET_CONN_H__

#include "websocket.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WebSocket handshake parameters
 * 
 * Parses the given URI and initializes the WebSocket connection parameters including
 * scheme, host, path, port, and TLS settings. Also sets up initial connection state.
 * 
 * @param[in,out] ws Pointer to WebSocket structure to be initialized
 * @param[in] uri WebSocket URI to connect to (e.g., "ws://example.com:8080/path")
 * @return OPERATE_RET 
 *         - OPRT_OK: Success
 *         - OPRT_COM_ERROR: Invalid URI parameters
 *         - Others: Other failures
 */
OPERATE_RET websocket_handshake_init(WEBSOCKET_S *ws, char *uri);

/**
 * @brief Start WebSocket handshake process
 * 
 * Performs the complete WebSocket handshake process including:
 * 1. Establishing TCP connection
 * 2. Sending handshake request
 * 3. Receiving and validating handshake response
 * 
 * This function is thread-safe and uses mutex for synchronization.
 * 
 * @param[in,out] ws Pointer to initialized WebSocket structure
 * @return OPERATE_RET
 *         - OPRT_OK: Handshake successful
 *         - OPRT_SOCK_CONN_ERR: Connection or handshake failed
 *         - Others: Other failures
 * 
 * @note If handshake fails, it will close the connection and clear DNS cache
 */
OPERATE_RET websocket_handshake_start(WEBSOCKET_S *ws);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /** !__WEBSOCKET_CONN_H__ */
