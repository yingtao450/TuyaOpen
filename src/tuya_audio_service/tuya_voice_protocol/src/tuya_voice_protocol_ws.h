/**
 * @file tuya_voice_protocol_ws.h
 * @brief Defines the WebSocket protocol implementation for Tuya's voice service.
 *
 * This source file provides the implementation of WebSocket-based voice protocol
 * functionality within the Tuya voice service framework. It includes the
 * initialization, configuration, and processing functions for voice data
 * transmission over WebSocket, which handles real-time bidirectional
 * communication between IoT devices and the cloud platform. The implementation
 * supports various voice features including ASR, TTS, and NLP processing through
 * WebSocket protocol. This file is essential for developers working on IoT
 * applications that require low-latency voice interaction and streaming
 * capabilities.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TUYA_IOT_VOICE_PROTOCOL_WS_H__
#define __TUYA_IOT_VOICE_PROTOCOL_WS_H__

#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_voice_protocol_upload.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the current voice request ID
 * 
 * This function retrieves the ID of the currently active voice request.
 * 
 * @param[out] request_id Buffer to store the current request ID
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success
 * @retval Other Error codes from internal implementation
 */
OPERATE_RET tuya_voice_get_current_request_id(char *request_id);

/**
 * @brief Initialize the WebSocket voice protocol
 * 
 * This function initializes the WebSocket voice protocol by setting up callbacks,
 * initializing the WebSocket client, and creating necessary synchronization primitives.
 * 
 * @param[in] cbs Pointer to structure containing callback functions
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success
 * @retval OPRT_INVALID_PARM Invalid parameters (cbs is NULL)
 * 
 * @note This function must be called before using any other WebSocket voice protocol functions
 */
OPERATE_RET tuya_voice_proto_ws_init(TUYA_VOICE_CBS_S *cbs);

/**
 * @brief Deinitialize the WebSocket voice protocol
 * 
 * This function cleans up resources used by the WebSocket voice protocol,
 * including releasing mutex and clearing protocol state.
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success
 * 
 * @note This function should be called when the WebSocket voice protocol is no longer needed
 */
OPERATE_RET tuya_voice_proto_ws_deinit(void);

/**
 * @brief Start the WebSocket client
 * 
 * This function starts the WebSocket client and prepares it for communication.
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success
 * @retval Other Error codes from WebSocket client implementation
 */
OPERATE_RET tuya_voice_proto_ws_client_start(void);

/**
 * @brief Stop the WebSocket client
 * 
 * This function stops the WebSocket client and terminates any ongoing communication.
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success
 * @retval Other Error codes from WebSocket client implementation
 */
OPERATE_RET tuya_voice_proto_ws_client_stop(void);

/**
 * @brief Delete the domain name configuration
 * 
 * This function removes the configured domain name from the WebSocket client.
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success
 * @retval Other Error codes from WebSocket client implementation
 * 
 * @note This operation may affect the ability to establish new connections
 */
OPERATE_RET tuya_voice_proto_ws_del_domain_name(void);


/**
 * @brief Send text content to TTS service through websocket protocol
 *
 * @details This function sends text content to the Text-to-Speech service for processing.
 *          The function performs the following operations:
 *          - Validates the input text parameter
 *          - Checks websocket connection status
 *          - Generates a unique request ID
 *          - Encodes the request using protobuf format
 *          - Sends the encoded data through websocket connection
 *
 * @param[in] p_tts_text  The text content to be processed by TTS service.
 *                        Must be a null-terminated string.
 *
 * @return OPERATE_RET @n
 * - OPRT_OK: Text sent successfully
 * - OPRT_INVALID_PARM: Invalid parameter (NULL text pointer)
 * - OPRT_COM_ERROR: Communication error, including:
 *                   - Websocket disconnection
 *                   - Protobuf encoding failure
 *                   - Websocket send failure
 * - OPRT_MALLOC_FAILED: Memory allocation failed for encoding buffer
 *
 * @note This function requires an active websocket connection before sending the text.
 *       The maximum text length is limited by the available memory for protobuf encoding.
 */
OPERATE_RET tuya_voice_proto_ws_get_tts_text(char *p_tts_text);

/**
 * @brief Request text-to-speech audio conversion with specific voice settings
 *
 * @details This function sends a text string to the TTS service for conversion to audio,
 *          allowing specification of a session ID and declaimer (voice type).
 *          The function uses protobuf encoding and websocket protocol for communication.
 *
 * @param[in] p_session_id  Session ID for the TTS request
 * @param[in] p_tts_text    The text string to be converted to speech
 * @param[in] p_declaimer   The voice type/declaimer to use for the conversion
 *
 * @return OPERATE_RET @n
 * - OPRT_OK: Success
 * - OPRT_INVALID_PARM: Invalid parameters (NULL pointers)
 * - OPRT_COM_ERROR: Communication error or encoding failed
 * - OPRT_MALLOC_FAILED: Memory allocation failed
 */
OPERATE_RET tuya_voice_proto_ws_get_tts_audio(char *p_session_id,
                                       char *p_tts_text,
                                       char *p_declaimer);
/**
 * @brief Initialize and start a voice upload session through websocket protocol
 *
 * @details This function initializes a new voice upload session with the specified parameters.
 *          It performs the following operations:
 *          - Validates input parameters
 *          - Checks websocket connection status
 *          - Generates a unique request ID for the session
 *          - Configures the upload context based on the audio format
 *          - Supports multiple audio formats (SPEEX, WAV, ULAW)
 *          - Handles multi-round dialogue through session ID
 *
 * @param[out] uploader      Pointer to store the created upload context
 * @param[in]  format        Audio format of the voice data to be uploaded:
 *                          - TUYA_VOICE_AUDIO_FORMAT_SPEEX: Speex format
 *                          - TUYA_VOICE_AUDIO_FORMAT_WAV: WAV format
 *                          - TUYA_VOICE_AUDIO_FORMAT_ULAW: μ-law format
 * @param[in]  target        Target type for the voice upload
 * @param[in]  p_session_id  Session ID for multi-round dialogue, can be NULL for single round
 * @param[in]  p_buf         Buffer containing format-specific parameters
 * @param[in]  buf_len       Length of the parameter buffer
 *
 * @return OPERATE_RET @n
 * - OPRT_OK: Upload session started successfully
 * - OPRT_INVALID_PARM: Invalid parameters (NULL uploader or invalid format)
 * - OPRT_COM_ERROR: Communication error or unsupported audio format
 * - OPRT_MALLOC_FAILED: Memory allocation failed for upload context
 *
 * @note The function requires an active websocket connection before starting the upload.
 *       The caller is responsible for managing the memory of p_session_id and p_buf.
 */
OPERATE_RET tuya_voice_proto_ws_upload_start(TUYA_VOICE_UPLOAD_T *uploader,
                                    TUYA_VOICE_AUDIO_FORMAT_E format,
                                    TUYA_VOICE_UPLOAD_TARGET_E target,
                                    char *p_session_id,
                                    uint8_t *p_buf,
                                    uint32_t buf_len);

/**
 * @brief Send voice data in an active upload session
 *
 * @details This function sends voice data chunks through the websocket connection.
 *          It encodes the data using protobuf and sends it as binary data.
 *          The function maintains an internal counter for the total amount of data sent.
 *
 * @param[in] uploader  The upload context created by tuya_voice_proto_ws_upload_start
 * @param[in] buf       Buffer containing the voice data to send
 * @param[in] len       Length of the voice data buffer
 *
 * @return OPERATE_RET @n
 * - OPRT_OK: Success
 * - OPRT_INVALID_PARM: Invalid parameter
 * - OPRT_COM_ERROR: Communication error
 * - OPRT_MALLOC_FAILED: Memory allocation failed
 */
OPERATE_RET tuya_voice_proto_ws_upload_send(TUYA_VOICE_UPLOAD_T uploader, uint8_t *buf, uint32_t len);

/**
 * @brief Stop an active voice upload session
 *
 * @details This function terminates a voice upload session. It can either perform
 *          a graceful shutdown by sending an ASR_END message, or force stop the
 *          session immediately. After stopping, it frees the upload context.
 *
 * @param[in] uploader    The upload context to be stopped
 * @param[in] force_stop  If TRUE, forcefully stop without sending ASR_END message
 *                        If FALSE, perform graceful shutdown
 *
 * @return OPERATE_RET @n
 * - OPRT_OK: Success
 * - OPRT_INVALID_PARM: Invalid parameter
 * - OPRT_COM_ERROR: Communication error
 * - OPRT_MALLOC_FAILED: Memory allocation failed during graceful shutdown
 */
OPERATE_RET tuya_voice_proto_ws_upload_stop(TUYA_VOICE_UPLOAD_T uploader, BOOL_T force_stop);

/**
 * @brief Get the message ID (request ID) of an active upload session
 *
 * @details This function retrieves the request ID associated with the current
 *          upload session and copies it to the provided buffer.
 *
 * @param[in]  uploader  The upload context to get the message ID from
 * @param[out] buffer    Buffer to store the message ID string
 * @param[in]  len       Length of the provided buffer
 *
 * @return OPERATE_RET @n
 * - OPRT_OK: Success
 * - OPRT_INVALID_PARM: Invalid uploader parameter
 */
OPERATE_RET tuya_voice_proto_ws_upload_get_message_id(TUYA_VOICE_UPLOAD_T uploader, char *buffer, int len);

/**
 * @brief Send a control command through websocket protocol
 *
 * @details This function sends a control command with the specified request ID.
 *          It encodes the command using protobuf and sends it through the websocket
 *          connection. The command is sent as a key-value pair in the options field.
 *
 * @param[in] request_id  The request ID associated with the control command
 * @param[in] command     The control command string to send
 *
 * @return OPERATE_RET @n
 * - OPRT_OK: Success
 * - OPRT_INVALID_PARM: Invalid parameters
 * - OPRT_COM_ERROR: Communication error or encoding failed
 * - OPRT_MALLOC_FAILED: Memory allocation failed
 */
OPERATE_RET tuya_voice_proto_ws_control(char *request_id, char *command);

/**
 * @brief Send a voice skill request through WebSocket protocol
 * 
 * This function sends a voice skill request with specified domain, intent and optional
 * parameters through WebSocket connection. The request is encoded using protobuf format.
 * 
 * @param[in] domain The domain of the skill request
 * @param[in] intent The intent of the skill request
 * @param[in] slots Optional JSON string containing slot information, can be NULL
 * @param[in] raw Optional raw data string, can be NULL
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success
 * @retval OPRT_INVALID_PARM Invalid parameters (domain or intent is NULL)
 * @retval OPRT_COM_ERROR Communication error or encoding error
 * @retval OPRT_MALLOC_FAILED Memory allocation failed
 * 
 * @note The function will generate a unique request ID for each call
 * @note The WebSocket connection must be established before calling this function
 */
OPERATE_RET tuya_voice_proto_ws_skill_request(char *domain, char *intent, char *slots, char *raw);

/**
 * @brief Request TTS stream conversion for given text
 *
 * @details This function creates a skill request for text-to-speech streaming.
 *          It formats the text into a slot parameter and sends it to the
 *          general voice playback skill.
 *
 * @param[in] tts_text  The text to be converted to speech (must not be NULL)
 *
 * @return OPERATE_RET @n
 * - OPRT_OK: TTS stream request sent successfully
 * - OPRT_INVALID_PARM: Invalid parameter (NULL text)
 * - OPRT_COM_ERROR: Communication error
 * - OPRT_MALLOC_FAILED: Memory allocation failed
 *
 * @note This is a wrapper function that calls tuya_voice_proto_ws_skill_request
 *       with predefined domain and intent values.
 */
OPERATE_RET tuya_voice_proto_ws_get_tts_stream(char *tts_text);

/**
 * @brief Interrupt current voice WebSocket request
 * 
 * This function interrupts the currently active voice request by sending an interrupt
 * control command through WebSocket connection. If there is no active request,
 * the function will return successfully without performing any operation.
 * 
 * @return OPERATE_RET
 * @retval OPRT_OK Success, either interrupted active request or no request to interrupt
 * @retval Other Error codes from tuya_voice_proto_ws_control function
 * 
 * @note This function checks if there is an active request before attempting to interrupt
 */
OPERATE_RET tuya_voice_proto_ws_interrupt(void);

/**
 * @brief Check if WebSocket connection is online
 * 
 * This function checks the current status of the WebSocket connection
 * to determine if it is online and available for communication.
 * 
 * @return BOOL_T
 * @retval TRUE WebSocket connection is online
 * @retval FALSE WebSocket connection is offline
 */
BOOL_T tuya_voice_proto_ws_is_online(void);
/**
 * @brief Disconnect the WebSocket connection
 * 
 * This function actively disconnects the current WebSocket connection.
 * Any ongoing communication will be terminated.
 * 
 * @note After disconnection, the connection needs to be re-established
 *       before any further communication can occur
 */
void tuya_voice_proto_ws_disconnect(void);
/**
 * @brief Set the keepalive interval for WebSocket connection
 * 
 * This function configures the keepalive interval for the WebSocket connection
 * to maintain the connection alive and detect disconnections.
 * 
 * @param[in] sec Keepalive interval in seconds
 * 
 * @note Setting a proper keepalive interval is important for maintaining
 *       a stable connection and detecting network issues early
 */
void tuya_voice_proto_ws_set_keepalive(uint32_t sec);

#ifdef __cplusplus
}
#endif

#endif /* __TUYA_IOT_VOICE_PROTOCOL_WS_H__ */
