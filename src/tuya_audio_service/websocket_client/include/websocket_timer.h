/**
 * @file websocket_timer.h
 * @brief Implements timer management functionality for WebSocket heartbeat mechanism
 *
 * This source file provides the implementation of timer-related functionality
 * for WebSocket heartbeat management. It includes functions for creating,
 * initializing, starting, stopping and releasing ping/pong timers. The
 * implementation supports both cyclic ping timer for sending heartbeat messages
 * and one-shot pong timer for monitoring response timeouts. This file is
 * essential for developers working on IoT applications that require reliable
 * WebSocket connections with proper connection health monitoring through
 * heartbeat mechanisms.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __WEBSOCKET_TIMER_H__
#define __WEBSOCKET_TIMER_H__

#include "websocket.h"
#include "tal_sw_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create heartbeat timers for websocket connection
 * 
 * This function creates two software timers for websocket heartbeat mechanism:
 * one for PING messages and another for PONG responses.
 * 
 * @param[in] ws Pointer to websocket structure
 * @param[in] ping_cb Callback function for PING timer events
 * @param[in] pong_cb Callback function for PONG timer events
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Timer creation successful
 * @retval OPRT_INVALID_PARM Invalid parameters (NULL pointers)
 */
OPERATE_RET websocket_hb_timer_create(WEBSOCKET_S *ws, TAL_TIMER_CB ping_cb, TAL_TIMER_CB pong_cb);

/**
 * @brief Release all heartbeat timers
 * 
 * This function deletes both PING and PONG timers and releases their resources.
 * Should be called during websocket cleanup.
 * 
 * @param[in] ws Pointer to websocket structure
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Timers released successfully
 * @retval OPRT_INVALID_PARM Invalid parameters or timers not created
 */
OPERATE_RET websocket_hb_timer_release(WEBSOCKET_S *ws);

/**
 * @brief Initialize PING timer settings
 * 
 * This function initializes the PING timer with specified interval for
 * periodic heartbeat checks.
 * 
 * @param[in] ws Pointer to websocket structure
 * @param[in] interval Time interval in milliseconds between PING messages
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Initialization successful
 * @retval OPRT_INVALID_PARM Invalid websocket pointer
 */
OPERATE_RET websocket_hb_ping_timer_init(WEBSOCKET_S *ws, TIME_MS interval);

/**
 * @brief Start the PING timer
 * 
 * This function starts the PING timer with previously configured interval
 * in cyclic mode to send periodic PING messages.
 * 
 * @param[in] ws Pointer to websocket structure
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Timer started successfully
 * @retval OPRT_INVALID_PARM Invalid parameters or timer not created
 */
OPERATE_RET websocket_hb_ping_timer_start(WEBSOCKET_S *ws);

/**
 * @brief Stop the PING timer
 * 
 * This function stops the PING timer if it's running. It will log a warning
 * if the timer is already stopped.
 * 
 * @param[in] ws Pointer to websocket structure
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Timer stopped successfully or already stopped
 * @retval OPRT_INVALID_PARM Invalid parameters or timer not created
 */
OPERATE_RET websocket_hb_ping_timer_stop(WEBSOCKET_S *ws);

/**
 * @brief Initialize PONG timer settings
 * 
 * This function initializes the PONG timer with specified timeout value
 * for monitoring PONG response timeouts.
 * 
 * @param[in] ws Pointer to websocket structure
 * @param[in] timeout Timeout value in milliseconds for PONG response
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Initialization successful
 * @retval OPRT_INVALID_PARM Invalid websocket pointer
 */
OPERATE_RET websocket_hb_pong_timer_init(WEBSOCKET_S *ws, TIME_MS timeout);

/**
 * @brief Start the PONG timer
 * 
 * This function starts the PONG timer in one-shot mode to monitor
 * the timeout for receiving PONG response.
 * 
 * @param[in] ws Pointer to websocket structure
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Timer started successfully
 * @retval OPRT_INVALID_PARM Invalid parameters or timer not created
 */
OPERATE_RET websocket_hb_pong_timer_start(WEBSOCKET_S *ws);

/**
 * @brief Stop the PONG timer
 * 
 * This function stops the PONG timer if it's running. It will log a warning
 * if the timer is already stopped.
 * 
 * @param[in] ws Pointer to websocket structure
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Timer stopped successfully or already stopped
 * @retval OPRT_INVALID_PARM Invalid parameters or timer not created
 */
OPERATE_RET websocket_hb_pong_timer_stop(WEBSOCKET_S *ws);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /** !__WEBSOCKET_TIMER_H__ */
