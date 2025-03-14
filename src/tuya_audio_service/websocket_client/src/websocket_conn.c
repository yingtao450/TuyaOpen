/**
 * @file websocket_conn.c
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

#include "websocket_utils.h"
#include "websocket_conn.h"
#include "websocket_netio.h"
#include "tal_security.h"
#include "mix_method.h"
#include "tal_network.h"
#include "tal_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define WS_PORT                         (80)
#define WSS_PORT                        (443)
#define WS_CLIENT_KEY_SIZE              (24)    // handshake client key size
#define WS_SERVER_KEY_SIZE              (28)    // handshake server key size
#define WS_HANDSHAKE_SEND_BUF_SIZE      (512)   // handshake send buffer size
#define WS_HANDSHAKE_RECV_BUF_SIZE      (512)   // handshake recv buffer size

static void websocket_parse_uri(char *uri,
                                char **scheme,
                                char **host,
                                char **path,
                                uint16_t *port,
                                BOOL_T *tls_enable)
{
    char *end = NULL;
    uint8_t unix_skt = 0;

    *port = 0;
    /* cut up the location into address, port and path */
    *scheme = uri;
    while (*uri && (*uri != ':' || uri[1] != '/' || uri[2] != '/'))
        uri++;
    if (!*uri) {
        end = uri;
        uri = (char *)*scheme;
        *scheme = end;
    } else {
        *uri = '\0';
        uri += 3;
    }
    if (*uri == '+') /* unix skt */
        unix_skt = 1;

    *host = uri;
    if (!strcmp(*scheme, "ws") || !strcmp(*scheme, "http")) {
        *port = WS_PORT;
        *tls_enable = FALSE;
    } else if (!strcmp(*scheme, "wss") || !strcmp(*scheme, "https")) {
        *port = WSS_PORT;
        *tls_enable = TRUE;
    }

    if (*uri == '[') {
        ++(*host);
        while (*uri && *uri != ']')
            uri++;
        if (*uri)
            *uri++ = '\0';
    } else
        while (*uri && *uri != ':' && (unix_skt || *uri != '/'))
            uri++;

    if (*uri == ':') {
        *uri++ = '\0';
        *port = (uint16_t)atoi(uri);
        while (*uri && *uri != '/')
            uri++;
    }
    *path = "/";
    if (*uri) {
        *uri++ = '\0';
        if (*uri)
            *path = uri;
    }
}

static BOOL_T __check_uri_info_validity(WEBSOCKET_S *ws)
{
    WS_CHECK_NULL_RET_VAL(ws, FALSE);
    WS_CHECK_NULL_RET_VAL(ws->uri, FALSE);
    WS_CHECK_NULL_RET_VAL(ws->path, FALSE);
    WS_CHECK_NULL_RET_VAL(ws->host, FALSE);
    WS_CHECK_BOOL_RET_VAL(ws->port == 0, FALSE);
    return TRUE;
}

static char *websocket_set_pathname(char *path)
{
    char *dst = NULL;
    WS_CHECK_NULL_RET_VAL(path, NULL);

    if ('/' == path[0]) {
        dst = websocket_string_dupcpy(path);
    } else {
        size_t len = strlen(path);
        WS_MALLOC_ERR_RET_VAL(dst, len + 2, NULL);
        snprintf(dst, len + 2, "/%s", path);
        dst[len + 1] = '\0';
    }

    return dst;
}

static OPERATE_RET websocket_gethostbyname(char *host, TUYA_IP_ADDR_T *ip)
{
    OPERATE_RET rt = OPRT_OK;
    WS_CHECK_NULL_RET(host);

    // if (isalpha(host[0])) {
    if(1){
        rt = tal_net_gethostbyname(host, ip);
        if (rt != UNW_SUCCESS) {
            PR_ERR("dns parser %s failed, rt: %d", host, rt);
            return OPRT_COM_ERROR;
        }
    } else {
        *ip = tal_net_str2addr(host);
    }
    PR_DEBUG("websocket server ip: %s", tal_net_addr2str(*ip));

    return OPRT_OK;
}

static OPERATE_RET websocket_generate_client_key(char *client_key)
{
    uint8_t random_data[16] = {0};
    int i = 0;
    WS_CHECK_NULL_RET(client_key);

    for (i = 0; i < sizeof(random_data); i++) {
        random_data[i] = tal_system_get_random(0xFF);
    }
    tuya_base64_encode(random_data, client_key, sizeof(random_data));
    PR_DEBUG("websocket client key: %s", client_key);

    return OPRT_OK;
}

static OPERATE_RET websocket_format_handshake(WEBSOCKET_S *ws, char *buf, char *client_key)
{
    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(buf);
    WS_CHECK_NULL_RET(client_key);

    char *hdshk_buf = buf;

    /** WARNING: don't support authentication */
    hdshk_buf += sprintf(hdshk_buf, "GET %s HTTP/1.1\r\n", ws->path);
    if (NULL != ws->host)
        hdshk_buf += sprintf(hdshk_buf, "Host: %s:%d\r\n", ws->host, ws->port);
    else
        hdshk_buf += sprintf(hdshk_buf, "Host:\r\n");
    if (NULL != ws->origin)
        hdshk_buf += sprintf(hdshk_buf, "Origin: %s\r\n", ws->origin);
    hdshk_buf += sprintf(hdshk_buf, "Upgrade: websocket\r\n");
    hdshk_buf += sprintf(hdshk_buf, "Connection: Upgrade\r\n");
    if (NULL != ws->sub_prot)
        hdshk_buf += sprintf(hdshk_buf, "Sec-WebSocket-Protocol: %s\r\n", ws->sub_prot);
    hdshk_buf += sprintf(hdshk_buf, "Sec-WebSocket-Key: %s\r\n", client_key);
    hdshk_buf += sprintf(hdshk_buf, "Sec-WebSocket-Version: 13\r\n");
    hdshk_buf += sprintf(hdshk_buf, "\r\n");

    PR_DEBUG("websocket %p handshake, client send:\r\n%s", ws, buf);

    return OPRT_OK;
}

static OPERATE_RET websocket_parse_handshake(WEBSOCKET_S *ws, char *buf, char *server_key)
{
    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(buf);
    WS_CHECK_NULL_RET(server_key);

    /** parse Sec-WebSocket-Accept: */
    char *accept_tab = "Sec-WebSocket-Accept: ";
    char *sign = strstr(buf, accept_tab);
    if (NULL == sign) {
        PR_ERR("websocket %p, %s not match", ws, accept_tab);
        return OPRT_NOT_FOUND;
    }

    char *accept_key = sign + strlen(accept_tab);
    strncpy(server_key, accept_key, WS_SERVER_KEY_SIZE);
    if (0 == strlen(server_key)) {
        PR_ERR("websocket %p, server_key is invalid", ws);
        return OPRT_COM_ERROR;
    }

    sign = strstr(sign, "\r\n");
    if (NULL == sign) {
        PR_ERR("websocket %p, \"\\r\\n\" not match", ws);
        return OPRT_COM_ERROR;
    } else {
        *sign = '\0';
    }

    return OPRT_OK;
}

static OPERATE_RET websocket_verify_server_key(char *client_key, char *server_key)
{
    WS_CHECK_NULL_RET(client_key);
    WS_CHECK_NULL_RET(server_key);

    // /** calculate server key base64(sha1(server_magic + client_key)) */
    char *websocket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    TKL_HASH_HANDLE sha1_ctx;
    uint8_t sha1_digest[20] = {0};
    char calculate_key[WS_SERVER_KEY_SIZE + 1] = {0};

    tal_sha1_create_init(&sha1_ctx);
    tal_sha1_starts_ret(sha1_ctx);
    tal_sha1_update_ret(sha1_ctx, (uint8_t *)client_key, strlen(client_key));
    tal_sha1_update_ret(sha1_ctx, (uint8_t *)websocket_guid, strlen(websocket_guid));
    tal_sha1_finish_ret(sha1_ctx,sha1_digest);
    tal_sha1_free(sha1_ctx);
    tuya_base64_encode(sha1_digest, calculate_key, sizeof(sha1_digest));
    PR_DEBUG("server key: %s, calculated key: %s", server_key, calculate_key);
    if (0 != strcmp(server_key, calculate_key)) {
        PR_ERR("server key not equal to calculated key");
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

static OPERATE_RET websocket_handshake_conn(WEBSOCKET_S *ws)
{
    OPERATE_RET rt = OPRT_OK;

    PR_DEBUG("websocket %p handshake connect", ws);

    WS_CHECK_NULL_RET(ws);

    WS_CALL_ERR_RET(websocket_gethostbyname(ws->host, &ws->hostaddr));
    WS_CALL_ERR_RET(websocket_netio_open(ws));
    WS_CALL_ERR_RET(websocket_netio_conn(ws));

    return OPRT_OK;
}

static OPERATE_RET websocket_handshake_send(WEBSOCKET_S *ws, char *client_key)
{
    OPERATE_RET rt = OPRT_OK;
    char send_buf[WS_HANDSHAKE_SEND_BUF_SIZE] = {0};

    PR_DEBUG("websocket %p handshake send", ws);

    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(client_key);

    WS_CALL_ERR_RET(websocket_generate_client_key(client_key));
    WS_CALL_ERR_RET(websocket_format_handshake(ws, send_buf, client_key));
    return websocket_netio_send_ext(ws, send_buf, strlen(send_buf));
}

static OPERATE_RET websocket_handshake_recv(WEBSOCKET_S *ws, char *client_key)
{
    OPERATE_RET rt = OPRT_OK;
    size_t total_recv_len = 0, once_recv_len = 0;
    char server_key[WS_SERVER_KEY_SIZE + 1] = {0};
    char recv_buf[WS_HANDSHAKE_RECV_BUF_SIZE] = {0};

    PR_DEBUG("websocket %p handshake recv", ws);

    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(client_key);

    for (;;) {
        rt = websocket_netio_recv(ws, (uint8_t *)recv_buf + total_recv_len,
                                  WS_HANDSHAKE_RECV_BUF_SIZE - total_recv_len, &once_recv_len);
        if (OPRT_OK != rt) {
            PR_ERR("websocket %p websocket_netio_recv error, rt:%d", ws, rt);
            return OPRT_RECV_ERR;
        }
        total_recv_len += once_recv_len;
        if (total_recv_len > WS_HANDSHAKE_RECV_BUF_SIZE) {
            PR_ERR("websocket %p total_recv_len:%d is too big", ws, total_recv_len);
            return OPRT_COM_ERROR;
        }

        if (strstr(recv_buf, "\r\n\r\n")) {
            recv_buf[total_recv_len] = '\0';
            PR_DEBUG("websocket %p handshake, server response:\r\n%s", ws, recv_buf);
            break;
        }
    }
    WS_CALL_ERR_RET(websocket_parse_handshake(ws, recv_buf, server_key));
    PR_DEBUG("got server_key %s",server_key);
    PR_DEBUG("client_key %s",client_key);
    WS_CALL_ERR_RET(websocket_verify_server_key(client_key, server_key));

    return OPRT_OK;
}

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
OPERATE_RET websocket_handshake_init(WEBSOCKET_S *ws, char *uri)
{
    char *scheme = NULL, *host = NULL, *path = NULL;

    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(uri);

    char *p_uri = websocket_string_dupcpy(uri);
    websocket_parse_uri(p_uri, &scheme, &host, &path, &ws->port, &ws->tls_enable);
    ws->uri = websocket_string_dupcpy(uri);
    ws->host = websocket_string_dupcpy(host);
    ws->path = websocket_set_pathname(path);
    ws->origin = websocket_string_dupcpy(NULL);
    ws->sub_prot = websocket_string_dupcpy(NULL);
    if (!__check_uri_info_validity(ws)) {
        PR_ERR("websocket %p uri para is invalid", ws);
        websocket_string_delete(&p_uri);
        return OPRT_COM_ERROR;
    }
    PR_DEBUG("websocket %p scheme:%s, host:%s, port:%d, enableTls:%d, path:%s, origin:%s, sub_prot:%s",
             ws, scheme, ws->host, ws->port, ws->tls_enable, ws->path,
             WS_SAFE_GET_STR(ws->origin), WS_SAFE_GET_STR(ws->sub_prot));
    websocket_string_delete(&p_uri);

    ws->sockfd = -1;
    ws->is_connected = FALSE;
    PR_DEBUG("websocket %p handshake initialized, default %s", ws, ws->is_connected ? "connected" : "disconnected");

    return OPRT_OK;
}

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
OPERATE_RET websocket_handshake_start(WEBSOCKET_S *ws)
{
    OPERATE_RET rt = OPRT_OK;
    WS_CHECK_NULL_RET(ws);
    WS_CHECK_NULL_RET(ws->mutex);

    WS_ASSERT(OPRT_OK == tal_mutex_lock(ws->mutex));
    if (ws->is_connected) {
        PR_WARN("websocket %p already connected, ignoring handshake.", ws);
        WS_ASSERT(OPRT_OK == tal_mutex_unlock(ws->mutex));
        return OPRT_OK;
    }

    do {
        char client_key[WS_CLIENT_KEY_SIZE + 1] = {0};

        if (OPRT_OK != (rt = websocket_handshake_conn(ws))) {
            PR_ERR("websocket %p websocket_handshake_conn error, rt:%d", ws, rt);
            rt = OPRT_SOCK_CONN_ERR;
            break;
        }
        if (OPRT_OK != (rt = websocket_handshake_send(ws, client_key))) {
            PR_ERR("websocket %p websocket_handshake_send error, rt:%d", ws, rt);
            rt = OPRT_SOCK_CONN_ERR;
            break;
        }
        if (OPRT_OK != (rt = websocket_handshake_recv(ws, client_key))) {
            PR_ERR("websocket %p websocket_handshake_recv error, rt:%d", ws, rt);
            rt = OPRT_SOCK_CONN_ERR;
            break;
        }
    } while (0);

    if (OPRT_OK != rt) {
        ws->is_connected = FALSE;
        if (ws->host) {
            // unw_clear_dns_cache(ws->host);
            PR_WARN("clear dns cache: %s", ws->host);
        }
        if (OPRT_OK != websocket_netio_close(ws)) {
            PR_ERR("websocket %p client websocket_netio_close failed %d", ws);
        }
        PR_ERR("websocket %p client handshake failed, connect to server failed %d, disconnected", ws, rt);
    } else {
        ws->is_connected = TRUE;
        PR_DEBUG("websocket %p client handshake successful, connect to server successful, connected", ws);
    }
    WS_ASSERT(OPRT_OK == tal_mutex_unlock(ws->mutex));

    return rt;
}
