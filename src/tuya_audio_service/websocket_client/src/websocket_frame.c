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

#include "websocket_utils.h"
#include "websocket_frame.h"
#include "websocket_netio.h"
#include "uni_random.h"
#include "tal_system.h"

static OPERATE_RET websocket_format_frame_header(BOOL_T fin, WEBSOCKET_FRAME_TYPE_E type, uint64_t len,
                                                 uint8_t *masking_key, uint8_t *headbuf, uint8_t *headlen)
{
    uint8_t i = 0, length = 0;
    WS_CHECK_NULL_RET(masking_key);
    WS_CHECK_NULL_RET(headbuf);
    WS_CHECK_NULL_RET(headlen);

    WEBSOCKET_FRAME_HEADER_S *frame_head = (WEBSOCKET_FRAME_HEADER_S *)headbuf;
    frame_head->fin = fin;
    frame_head->rsv = 0;
    frame_head->opcode = type;
    frame_head->mask = TRUE;

    length = sizeof(WEBSOCKET_FRAME_HEADER_S);

    if (len <= 125) {
        frame_head->payload_len = len;
    } else if (len <= 65535) {
        frame_head->payload_len = 126;
        WS_STOR_BE16(len, frame_head->ext_payload_len);
        length += sizeof(uint16_t);
    } else {
        frame_head->payload_len = 127;
        WS_STOR_BE64(len, frame_head->ext_payload_len);
        length += sizeof(uint64_t);
    }

    for (i = 0; i < WS_MASKING_KEY_SIZE; i++) {
        masking_key[i] = (uint8_t)uni_random_range(0xFF);
    }
    memcpy(headbuf + length, masking_key, WS_MASKING_KEY_SIZE);

    *headlen = length + WS_MASKING_KEY_SIZE;

    WS_DEBUG("websocket send: fin:%u, opcode:%x, payloadlen:%d, masking_key: 0x%x%x%x%x, headlen:%u, datelen:%lu",
             frame_head->fin, frame_head->opcode, frame_head->payload_len, masking_key[0], masking_key[1],
             masking_key[2], masking_key[3], *headlen, len);

    return OPRT_OK;
}

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
                                 void *data, size_t len, BOOL_T first, BOOL_T final)
{
    WEBSOCKET_FRAME_TYPE_E frame_type;
    uint8_t headbuf[WS_FRAME_HEADER_SIZE] = {0}, headlen = 0;
    uint8_t masking_key[WS_MASKING_KEY_SIZE] = {0};
    OPERATE_RET rt = OPRT_OK;
    WS_CHECK_NULL_RET(ws);

    frame_type = (!first) ? WS_FRAME_TYPE_CONTINUATION : type;
    rt = websocket_format_frame_header(final, frame_type, (uint64_t)len, masking_key, headbuf, &headlen);
    if (OPRT_OK != rt) {
        PR_ERR("websocket %p format header error, rt:%d", ws, rt);
        return OPRT_SEND_ERR;
    }

    if ((0 != len) && (NULL != (char *)data)) {
        size_t i = 0;
        char *buffer = NULL;
        size_t buffer_size = len + headlen;
        WS_MALLOC_ERR_RET(buffer, buffer_size);
        memcpy(buffer, headbuf, headlen);
        memcpy(buffer + headlen, (uint8_t*)data, len);
        for (i = 0; i < len; i++) {
            buffer[headlen + i] ^= masking_key[i % 4]; //convert unmasked data into masked data
        }
        rt = websocket_netio_send_lock(ws, buffer, buffer_size);
        if (OPRT_OK != rt) {
            WS_SAFE_FREE(buffer);
            PR_ERR("websocket %p websocket_send_frame error, rt:%d", ws, rt);
            return OPRT_SEND_ERR;
        }
        WS_SAFE_FREE(buffer);
    } else {
        rt = websocket_netio_send_lock(ws, headbuf, headlen);
        if (OPRT_OK != rt) {
            PR_ERR("websocket %p websocket send header error, rt:%d", ws, rt);
            return OPRT_SEND_ERR;
        }
    }

    return OPRT_OK;
}

static BOOL_T websocket_check_opcode_valid(uint8_t opcode)
{
    if (opcode != WS_FRAME_TYPE_CONTINUATION && opcode != WS_FRAME_TYPE_TEXT &&
        opcode != WS_FRAME_TYPE_BINARY && opcode != WS_FRAME_TYPE_CLOSE &&
        opcode != WS_FRAME_TYPE_PING && opcode != WS_FRAME_TYPE_PONG) {
        return FALSE;
    }
    return TRUE;
}

static OPERATE_RET websocket_parse_frame_header(WEBSOCKET_FRAME_HEADER_S *frame_head, uint64_t *len)
{
    WS_CHECK_NULL_RET(frame_head);
    WS_CHECK_NULL_RET(len);

    if (!websocket_check_opcode_valid(frame_head->opcode)) {
        PR_ERR("websocket frame type error, opcode:%d", frame_head->opcode);
        return OPRT_COM_ERROR;
    }
    if (frame_head->rsv != 0) {
        PR_ERR("websocket frame resverd error, rsv:%d", frame_head->rsv);
        return OPRT_COM_ERROR;
    }
    if (frame_head->mask != 0) {
        PR_ERR("websocket mask value must be 0, mask:%d", frame_head->mask);
        return OPRT_COM_ERROR;
    }

    switch (frame_head->payload_len) {
    case 126: *len = WS_LOAD_BE16(frame_head->ext_payload_len); break;
    case 127: *len = WS_LOAD_BE64(frame_head->ext_payload_len); break;
    default : *len = frame_head->payload_len; break;
    }

    WS_DEBUG("websocket recv: fin:%u, opcode:%u, payloadlen:%d, datalen:%d",
             frame_head->fin, frame_head->opcode, frame_head->payload_len, *len);

    return OPRT_OK;
}

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
OPERATE_RET websocket_recv_frame(WEBSOCKET_S *ws, WEBSOCKET_FRAME_RECV_CB frame_recv_cb)
{
    size_t headlen = sizeof(WEBSOCKET_FRAME_HEADER_S);
    uint8_t headbuf[WS_FRAME_HEADER_SIZE] = {0};
    WEBSOCKET_FRAME_HEADER_S *frame_head = (WEBSOCKET_FRAME_HEADER_S *)headbuf;
    OPERATE_RET rt = OPRT_OK;
    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(frame_recv_cb);

    rt = websocket_netio_recv_ext(ws, headbuf, headlen);
    if (OPRT_OK != rt) {
        PR_ERR("websocket %p websocket_netio_recv_ext error, rt:%d", ws, rt);
        return OPRT_RECV_ERR;
    }

    size_t ext_payload_len = 0;
    switch (frame_head->payload_len) {
        case 126: ext_payload_len = sizeof(uint16_t); break;
        case 127: ext_payload_len = sizeof(uint64_t); break;
        default : ext_payload_len = 0; break;
    }
    if (ext_payload_len > 0) {
        rt = websocket_netio_recv_ext(ws, headbuf+headlen, ext_payload_len);
        if (OPRT_OK != rt) {
            PR_ERR("websocket %p websocket_netio_recv_ext error, rt:%d", ws, rt);
            return OPRT_RECV_ERR;
        }
        headlen += ext_payload_len;
    }

    uint64_t data_len = 0;
    rt = websocket_parse_frame_header(frame_head, &data_len);
    if (OPRT_OK != rt) {
        PR_ERR("websocket %p websocket_parse_frame_header error, rt:%d", ws, rt);
        return OPRT_COM_ERROR;
    }

    if (data_len == 0) {
        if (frame_recv_cb) {
            frame_recv_cb(ws, frame_head->opcode, (BOOL_T)frame_head->fin, NULL, 0);
        }
    } else {
        uint8_t *data = NULL;
        WS_MALLOC_ERR_RET(data, data_len);
        rt = websocket_netio_recv_ext(ws, data, data_len);
        if (OPRT_OK != rt) {
            WS_SAFE_FREE(data);
            PR_ERR("websocket %p websocket_netio_recv_ext error, rt:%d", ws, rt);
            return OPRT_RECV_ERR;
        }
        if (frame_recv_cb) {
            frame_recv_cb(ws, frame_head->opcode, (BOOL_T)frame_head->fin, data, data_len);
        }
        WS_SAFE_FREE(data);
    }

    return OPRT_OK;
}

