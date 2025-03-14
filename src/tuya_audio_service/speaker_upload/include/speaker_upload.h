/**
 * @file speaker_upload.h
 * @brief Defines the speaker audio upload interface for Tuya's audio service.
 *
 * This source file provides the implementation of speaker audio upload
 * functionality within the Tuya audio service framework. It includes the
 * initialization, configuration, and processing functions for audio upload,
 * which handles the capture and transmission of speaker audio data to the
 * cloud platform. The implementation supports various upload parameters and
 * protocols to ensure reliable audio data transmission. This file is essential
 * for developers working on IoT applications that require remote audio
 * monitoring and speaker feedback analysis capabilities.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef SPEAKER_UPLOAD_H
#define SPEAKER_UPLOAD_H

#include "tuya_iot_config.h"
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "tuya_voice_protocol_upload.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum speaker_upload_stat {
    SPEAKER_UP_STAT_INIT    = 0,
    SPEAKER_UP_STAT_ERR     = 1,
    SPEAKER_UP_STAT_NET_ERR = 2,
    SPEAKER_UP_STAT_ENC_ERR = 3,
} SPEAKER_UPLOAD_STAT_E;

typedef struct speaker_encode_info {
    TUYA_VOICE_AUDIO_FORMAT_E   encode_type;                        /*< encode type */
    struct {
        uint8_t         channels;                           /*< pcm channels */
        uint32_t          rate;                               /*< pcm samplerate */
        uint16_t        bits_per_sample;                    /*< pcm bits_per_sample */
    } info;
    char session_id[TUYA_VOICE_MESSAGE_ID_MAX_LEN + 1];     /*< dailog session id */
} SPEAKER_ENCODE_INFO_S;

typedef struct speaker_media_encoder SPEAKER_MEDIA_ENCODER_S;

typedef void (*SPEAKER_UPLOAD_REPORT_STAT_CB)(SPEAKER_UPLOAD_STAT_E stat, void *userdata);

typedef struct speaker_upload_config {
    SPEAKER_ENCODE_INFO_S           params;
    SPEAKER_UPLOAD_REPORT_STAT_CB   report_stat_cb;
    void                           *userdata;
} SPEAKER_UPLOAD_CONFIG_S;

/**
 * @brief Initialize the speaker upload interface
 *
 * This function initializes the speaker upload system with the provided configuration.
 * It sets up the upload manager, creates necessary mutexes and timers, and
 * initializes the encoder subsystem.
 *
 * @param[in] config Pointer to the upload configuration structure
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Initialization successful
 * @retval OPRT_INVALID_PARM Invalid configuration pointer
 * @retval OPRT_COM_ERROR Mutex or timer creation failed
 */
OPERATE_RET speaker_intf_upload_init(SPEAKER_UPLOAD_CONFIG_S *config);

/**
 * @brief Register a new media encoder
 *
 * This function registers a new media encoder with the speaker upload system.
 * The encoder will be available for use in subsequent upload sessions.
 *
 * @param[in] encoder Pointer to the encoder structure to register
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Registration successful
 * @retval Other Error codes from encoder registration process
 */
OPERATE_RET speaker_intf_encode_register(SPEAKER_MEDIA_ENCODER_S *encoder);


/**
 * @brief Start a new media upload session
 *
 * This function initializes and starts a new media upload session with the specified
 * session ID. It sets up the encoder, initializes the upload context, and begins
 * the upload process. If there's an existing upload session, it will be forcefully
 * stopped before starting the new one.
 *
 * @param[in] session_id Unique identifier for the upload session
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Upload session started successfully
 * @retval OPRT_INVALID_PARM Invalid session ID (NULL)
 * @retval OPRT_COM_ERROR Encoder start or upload initialization failed
 */
OPERATE_RET speaker_intf_upload_media_start(const char *session_id);

/**
 * @brief Send audio data for upload
 *
 * This function processes and sends audio data to the upload stream. It handles
 * the encoding of raw audio data and manages the upload timing statistics.
 *
 * @param[in] p_buf Pointer to the audio data buffer
 * @param[in] buf_len Length of the audio data in bytes
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Data sent successfully
 * @retval Other Error codes from encoding or upload process
 */
OPERATE_RET speaker_intf_upload_media_send(const uint8_t *p_buf, uint32_t buf_len);

/**
 * @brief Stop the current media upload session
 *
 * This function stops the current media upload session. It can either perform
 * a graceful shutdown or force stop the upload process. It cleans up encoder
 * resources and upload context.
 *
 * @param[in] is_force_stop TRUE for immediate force stop, FALSE for graceful shutdown
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Upload stopped successfully
 * @retval Other Error codes from stopping upload process
 */
OPERATE_RET speaker_intf_upload_media_stop(BOOL_T is_force_stop);

/**
 * @brief Retrieve the message ID for the current upload session
 *
 * This function gets the unique message identifier associated with the current
 * upload session. The message ID is copied into the provided buffer.
 *
 * @param[out] buffer Buffer to store the message ID
 * @param[in] len Maximum length of the buffer
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Message ID retrieved successfully
 * @retval Other Error codes from message ID retrieval process
 */
OPERATE_RET speaker_intf_upload_media_get_message_id(char *buffer, int len);

/**
 * @brief: upload config default
 */
#define SPEAKER_UPLOAD_CONFIG_DEFAULT()         \
    SPEAKER_UPLOAD_CONFIG_FOR_SPEEX()

/**
 * @brief: upload config, 16KHz S16_LE 1CHS speex
 */
#define SPEAKER_UPLOAD_CONFIG_FOR_SPEEX()       \
{                                               \
    .params = {                                 \
        .encode_type            = TUYA_VOICE_AUDIO_FORMAT_SPEEX,     \
        .info = {                               \
            .channels           = 1,            \
            .rate               = 16000,        \
            .bits_per_sample    = 16,           \
        },                                      \
    },                                          \
    .report_stat_cb             = NULL,         \
    .userdata                   = NULL,         \
}

/**
 * @brief: upload config, 16KHz S16_LE 1CHS opus
 */
#define SPEAKER_UPLOAD_CONFIG_FOR_OPUS()        \
{                                               \
    .params = {                                 \
        .encode_type            = TUYA_VOICE_AUDIO_FORMAT_OPUS,      \
        .info = {                               \
            .channels           = 1,            \
            .rate               = 16000,        \
            .bits_per_sample    = 16,           \
        },                                      \
    },                                          \
    .report_stat_cb             = NULL,         \
    .userdata                   = NULL,         \
}

#ifdef __cplusplus
}
#endif

#endif /** !SPEAKER_UPLOAD_H */
