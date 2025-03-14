/**
 * @file websocket_frame.c
 * @brief Implements WebSocket frame handling and message formatting functionality
 *
 * This source file provides the implementation of WebSocket frame processing
 * within the WebSocket protocol framework. It includes functions for frame
 * construction, parsing, masking operations, and payload handling. The
 * implementation supports all WebSocket frame types including text, binary,
 * control frames (ping/pong, close), and fragmented messages. It handles
 * frame headers, masking keys, payload lengths, and ensures proper frame
 * formatting according to the WebSocket protocol specification (RFC 6455).
 * This file is essential for developers working on IoT applications that
 * require reliable WebSocket communication with proper frame-level message
 * handling.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __WEBSOCKET_FRAME_H__
#define __WEBSOCKET_FRAME_H__

#include "websocket.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WS_FRAME_HEADER_SIZE            (10) // frame header size
#define WS_MASKING_KEY_SIZE             (4) // masking key size

/**
 * @brief WebSocket ANBF description
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-------+-+-------------+-------------------------------+
    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    | |1|2|3|       |K|             |                               |
    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    |     Extended payload length continued, if payload len == 127  |
    + - - - - - - - - - - - - - - - +-------------------------------+
    |                               |Masking-key, if MASK set to 1  |
    +-------------------------------+-------------------------------+
    | Masking-key (continued)       |          Payload Data         |
    +-------------------------------- - - - - - - - - - - - - - - - +
    :                     Payload Data continued ...                :
    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    |                     Payload Data continued ...                |
    +---------------------------------------------------------------+
 **/

/**
 * @brief WebSocket frame types
 * ---- %x0 denotes a continuation frame
 * ---- %x1 denotes a text frame
 * ---- %x2 denotes a binary frame
 * ---- %x3-7 are reserved for further non-control frames
 * ---- %x8 denotes a connection close
 * ---- %x9 denotes a ping
 * ---- %xA denotes a pong
 * ---- %xB-F are reserved for further control frames
 * ---- declare: 0 8 9 A are control frames, 1 2 are data frames(non-control frames)
 **/
typedef enum {
   WS_FRAME_TYPE_CONTINUATION           = 0x00,
   WS_FRAME_TYPE_TEXT                   = 0x01,
   WS_FRAME_TYPE_BINARY                 = 0x02,
   WS_FRAME_TYPE_CLOSE                  = 0x08,
   WS_FRAME_TYPE_PING                   = 0x09,
   WS_FRAME_TYPE_PONG                   = 0x0A,
} WEBSOCKET_FRAME_TYPE_E;

/**
 * @brief WebSocket frame header
 **/
typedef struct {
#if defined(LITTLE_END) && (LITTLE_END==1)
    uint8_t opcode:4,
            rsv:3,
            fin:1;
    uint8_t payload_len:7,
            mask:1;
#else
    uint8_t fin:1,
            rsv:3,
            opcode:4;
    uint8_t mask:1,
            payload_len:7;
#endif
    uint8_t ext_payload_len[0];
} __attribute__((packed)) WEBSOCKET_FRAME_HEADER_S;

typedef void (*WEBSOCKET_FRAME_RECV_CB)(WEBSOCKET_S *ws, WEBSOCKET_FRAME_TYPE_E type, BOOL_T final, void *data, size_t len);

/**
 * @brief Send a WebSocket frame with specified parameters
 *
 * This function constructs and sends a WebSocket frame with the given data and frame parameters.
 * It handles both masked and unmasked frames, supports fragmentation, and performs the necessary
 * data masking as per WebSocket protocol specifications.
 *
 * @param[in] ws Pointer to the WebSocket structure
 * @param[in] type Type of the WebSocket frame (e.g., text, binary, ping, pong)
 * @param[in] data Pointer to the data to be sent in the frame
 * @param[in] len Length of the data in bytes
 * @param[in] first Boolean indicating if this is the first frame in a fragmented message
 * @param[in] final Boolean indicating if this is the final frame in a fragmented message
 *
 * @return OPERATE_RET
 *         - OPRT_OK: Frame sent successfully
 *         - OPRT_INVALID_PARM: Invalid parameters (NULL pointer)
 *         - OPRT_SEND_ERR: Error occurred during frame sending
 *
 * @note For fragmented messages, set first=TRUE for the first frame, first=FALSE for continuation
 *       frames, and final=TRUE for the last frame in the sequence.
 */
OPERATE_RET websocket_send_frame(WEBSOCKET_S *ws, WEBSOCKET_FRAME_TYPE_E type,
                                 void *data, size_t len, BOOL_T first, BOOL_T final);

/**
 * @brief Receive and process a WebSocket frame
 *
 * This function reads and processes an incoming WebSocket frame, including:
 * 1. Reading the frame header
 * 2. Handling extended payload lengths (16-bit and 64-bit)
 * 3. Parsing the frame header
 * 4. Processing the frame payload data
 * 5. Invoking the callback function with the received frame data
 *
 * @param[in] ws Pointer to the WebSocket structure
 * @param[in] frame_recv_cb Callback function to handle received frame data
 *                         The callback receives:
 *                         - WebSocket structure pointer
 *                         - Frame opcode
 *                         - FIN flag indicating if this is the final frame
 *                         - Pointer to payload data (NULL if empty)
 *                         - Length of payload data
 *
 * @return OPERATE_RET
 *         - OPRT_OK: Frame received and processed successfully
 *         - OPRT_INVALID_PARM: Invalid parameters (NULL pointers)
 *         - OPRT_RECV_ERR: Error occurred during frame reception
 *         - OPRT_COM_ERROR: Error parsing frame header
 *         - OPRT_MALLOC_FAILED: Memory allocation failure
 *
 * @note The callback function is responsible for processing the frame data
 *       before this function returns, as the data buffer will be freed
 */
OPERATE_RET websocket_recv_frame(WEBSOCKET_S *ws, WEBSOCKET_FRAME_RECV_CB frame_recv_cb);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /** !__WEBSOCKET_FRAME_H__ */
