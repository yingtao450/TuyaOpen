/**
 * @file websocket_netio.h
 * @brief Implements network I/O operations for WebSocket communication
 *
 * This source file provides the implementation of low-level network I/O
 * functionality for WebSocket communications. It includes functions for
 * socket operations, TLS/SSL handling, raw data transmission, and connection
 * management. The implementation supports both secure and non-secure
 * connections, timeout handling, connection state management, and error
 * recovery mechanisms. This file is essential for developers working on IoT
 * applications that require reliable network communication with proper
 * error handling and security features through the WebSocket protocol.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __WEBSOCKET_NETIO_H__
#define __WEBSOCKET_NETIO_H__

#include "websocket.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and initialize a websocket network connection
 * 
 * This function creates a TCP socket and configures it with appropriate settings for websocket communication.
 * It sets up socket options including port reuse, disabling Nagle algorithm, blocking mode, and keepalive if specified.
 * 
 * @param[in] ws Pointer to the websocket structure
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success
 * @retval OPRT_SOCK_ERR Socket creation failed
 * @retval OPRT_SET_SOCK_ERR Socket configuration failed
 */
OPERATE_RET websocket_netio_open(WEBSOCKET_S *ws);

/**
 * @brief Close the WebSocket connection gracefully
 *
 * Performs a clean WebSocket closure by:
 * 1. Sending a close frame with optional status code and reason
 * 2. Waiting for the close frame response from the server
 * 3. Closing the underlying TCP connection
 *
 * @param[in] ws_handle Handle to the WebSocket instance
 * @param[in] status_code Status code indicating close reason (optional)
 * @param[in] reason Text description of close reason (optional)
 *
 * @return OPERATE_RET
 *         - OPRT_OK: Connection closed successfully
 *         - OPRT_INVALID_PARM: Invalid WebSocket handle
 *         - OPRT_NOT_CONNECTED: WebSocket already disconnected
 *         - OPRT_CLOSE_FAILED: Error during closure process
 *         - Others: Other closure errors
 *
 * @note This function will wait for the server's close frame or timeout
 *       before returning
 */
OPERATE_RET websocket_netio_close(WEBSOCKET_S *ws);

/**
 * @brief Establish a websocket network connection
 * 
 * This function establishes a network connection for the websocket. It first creates a TCP connection
 * to the specified host and port. If TLS is enabled, it will also establish a secure TLS connection
 * with the appropriate security configuration based on the TUYA_SECURITY_LEVEL.
 * 
 * @param[in] ws Pointer to the websocket structure containing connection parameters
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Connection established successfully
 * @retval OPRT_SOCK_CONN_ERR Connection failed (either TCP or TLS)
 */
OPERATE_RET websocket_netio_conn(WEBSOCKET_S *ws);

/**
 * @brief Monitor websocket socket for read events
 * 
 * This function uses select() to monitor the websocket socket for read events.
 * It waits for data to be available for reading within the specified timeout period.
 * 
 * @param[in] ws Pointer to the websocket structure
 * @param[in] timeout_ms Timeout value in milliseconds
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Select operation successful
 * @retval OPRT_SOCK_ERR Invalid socket
 */
OPERATE_RET websocket_netio_select_read(WEBSOCKET_S *ws, uint32_t timeout_ms);

/**
 * @brief Send data through websocket connection
 * 
 * This function sends data through the websocket connection. If TLS is enabled,
 * it will use TLS write operation; otherwise, it will use raw socket send.
 * 
 * @param[in] ws Pointer to the websocket structure
 * @param[in] buf Buffer containing data to send
 * @param[in] len Length of data to send
 * 
 * @return OPERATE_RET
 * @retval >0 Number of bytes sent
 * @retval <=0 Send operation failed
 */
OPERATE_RET websocket_netio_send(WEBSOCKET_S *ws, uint8_t *buf, size_t len);

/**
 * @brief Extended send function to ensure complete data transmission
 * 
 * This function ensures that all data is sent by repeatedly calling websocket_netio_send
 * until all bytes are transmitted or an error occurs.
 * 
 * @param[in] ws Pointer to the websocket structure
 * @param[in] data Buffer containing data to send
 * @param[in] len Length of data to send
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK All data sent successfully
 * @retval OPRT_SEND_ERR Send operation failed
 */
OPERATE_RET websocket_netio_send_ext(WEBSOCKET_S *ws, void *data, size_t len);

/**
 * @brief Thread-safe send function with mutex protection
 * 
 * This function provides a thread-safe way to send data by using mutex locking.
 * It also checks connection status before sending data.
 * 
 * @param[in] ws Pointer to the websocket structure
 * @param[in] data Buffer containing data to send
 * @param[in] len Length of data to send
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Send operation successful
 * @retval OPRT_SOCK_CONN_ERR Connection lost
 */
OPERATE_RET websocket_netio_send_lock(WEBSOCKET_S *ws, void *data, size_t len);

/**
 * @brief Receive data from websocket connection
 * 
 * This function receives data from the websocket connection. If TLS is enabled,
 * it will use TLS read operation; otherwise, it will use raw socket receive.
 * 
 * @param[in] ws Pointer to the websocket structure
 * @param[out] buf Buffer to store received data
 * @param[in] size Size of the buffer
 * @param[out] recv_len Pointer to store the number of bytes received
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Receive operation successful
 * @retval OPRT_COM_ERROR TLS read error
 * @retval OPRT_INVALID_PARM Invalid parameters
 */
OPERATE_RET websocket_netio_recv(WEBSOCKET_S *ws, uint8_t *buf, size_t size, size_t *recv_len);

/**
 * @brief Extended receive function to ensure complete data reception
 * 
 * This function ensures that the specified amount of data is received by repeatedly
 * calling websocket_netio_recv until all bytes are received or an error occurs.
 * 
 * @param[in] ws Pointer to the websocket structure
 * @param[out] buf Buffer to store received data
 * @param[in] len Expected length of data to receive
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK All data received successfully
 * @retval OPRT_RECV_ERR Receive operation failed
 */
OPERATE_RET websocket_netio_recv_ext(WEBSOCKET_S *ws, uint8_t *buf, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /** !__WEBSOCKET_NETIO_H__ */
