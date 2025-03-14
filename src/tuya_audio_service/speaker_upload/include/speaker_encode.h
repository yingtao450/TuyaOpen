/**
 * @file speaker_encode.h
 * @brief Defines the speaker audio encoding interface for Tuya's audio service.
 *
 * This source file provides the implementation of speaker audio encoding
 * functionality within the Tuya audio service framework. It includes the
 * initialization, configuration, and processing functions for speaker encoding,
 * which handles the compression and formatting of audio data from speakers.
 * The implementation supports various encoding parameters and buffer management
 * mechanisms for efficient data handling. This file is essential for developers
 * working on IoT applications that require real-time speaker audio capture,
 * encoding, and transmission capabilities.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef SPEAKER_ENCODE_H
#define SPEAKER_ENCODE_H

#include "speaker_encode_types.h"
#include "speaker_upload_internal.h"
#include "speaker_upload.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SPEAKER_ENCODER_NUM     5

typedef OPERATE_RET (*ENCODER_HANDLER_CB)(void *p_encoder);

typedef struct {
    TUYA_VOICE_AUDIO_FORMAT_E   type;
    ENCODER_HANDLER_CB          handler;
    SPEAKER_MEDIA_ENCODER_S    *encoder;
} SPEAKER_ENCODER_HANDLER_S;

typedef struct {
    SPEAKER_ENCODER_HANDLER_S   encoder_arr[MAX_SPEAKER_ENCODER_NUM];
    uint32_t                      encoder_cnt;
} SPEAKER_ENCODER_S;

/**
 * @brief Initialize the speaker encode module
 *
 * This function initializes the speaker encode module. Currently, it serves as a
 * placeholder for future initialization requirements.
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Always returns success
 */
OPERATE_RET speaker_encode_init(void);

/**
 * @brief Register a new speaker encoder callback
 *
 * This function registers a new encoder instance in the encoder registry. It allocates
 * memory for the encoder and stores its configuration for later use.
 *
 * @param[in] encoder Pointer to the encoder structure to register
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Registration successful
 * @retval OPRT_INVALID_PARM Invalid encoder pointer or registry full
 */
OPERATE_RET speaker_encode_register_cb(SPEAKER_MEDIA_ENCODER_S *encoder);

/**
 * @brief Encode audio data for speaker upload
 *
 * This function processes raw audio data through the configured encoder for speaker upload.
 * It validates the input parameters and calls the encoder's encode function to process
 * the audio buffer.
 *
 * @param[in] p_upload Pointer to the media upload task structure
 * @param[in] p_buf Pointer to the input audio buffer to be encoded
 * @param[in] buf_len Length of the input audio buffer in bytes
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Encoding successful
 * @retval OPRT_INVALID_PARM Invalid parameters (NULL pointers)
 * @retval Other Encoding error codes from the encoder
 */
OPERATE_RET speaker_encode(MEDIA_UPLOAD_TASK_S *p_upload, uint8_t *p_buf, uint32_t buf_len);

/**
 * @brief Free resources associated with the speaker encoder
 *
 * This function releases resources used by the speaker encoder, including sending
 * any remaining buffered data and cleaning up the encoder instance. It also handles
 * the closure of test files if enabled.
 *
 * @param[in] p_upload Pointer to the media upload task structure
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Resources freed successfully
 * @retval OPRT_INVALID_PARM Invalid upload task pointer
 */
OPERATE_RET speaker_encode_free(MEDIA_UPLOAD_TASK_S *p_upload);

/**
 * @brief Start the speaker encoding process
 *
 * This function initializes and starts the speaker encoder based on the provided
 * parameters. It sets up the encoder configuration, initializes the encoder instance,
 * and prepares it for processing audio data.
 *
 * @param[in] p_upload Pointer to the media upload task structure
 * @param[in] p_param Pointer to the encoder parameters structure
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Encoder started successfully
 * @retval OPRT_INVALID_PARM Invalid parameters
 * @retval OPRT_COM_ERROR Encoder initialization failed
 */
OPERATE_RET speaker_encode_start(MEDIA_UPLOAD_TASK_S *p_upload, const SPEAKER_ENCODE_INFO_S *p_param);

#ifdef __cplusplus
}
#endif

#endif /** !SPEAKER_ENCODE_H */
