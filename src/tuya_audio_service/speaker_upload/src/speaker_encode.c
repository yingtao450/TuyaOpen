/**
 * @file speaker_encode.c
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

#include "speaker_encode.h"
#include "speaker_upload.h"
#include "tal_api.h"

static SPEAKER_ENCODER_S s_speaker_encoder = {0};

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
OPERATE_RET speaker_encode(MEDIA_UPLOAD_TASK_S *p_upload, uint8_t *p_buf, uint32_t buf_len)
{
    int count = 0;
    OPERATE_RET ret = OPRT_OK;

    if (NULL == p_buf || NULL == p_upload) {
        PR_ERR("invalid params");
        return OPRT_INVALID_PARM;
    }

    /** NOTE: internal do data callback, write enc data to upload send */
    if (p_upload->encoder.encoder_encode)
        count = p_upload->encoder.encoder_encode(&p_upload->encoder, p_upload, p_buf, buf_len);
    return (ret = (count >= 0) ? OPRT_OK : count);
}

static OPERATE_RET speaker_encode_result_write(SPEAKER_MEDIA_ENCODER_S *p_encoder,
        void *private_data, const unsigned char *p_buffer, unsigned int size)
{
    OPERATE_RET ret = OPRT_OK;
    MEDIA_UPLOAD_TASK_S *p_upload = (MEDIA_UPLOAD_TASK_S *)private_data;

    if (NULL == p_encoder || NULL == p_buffer || NULL == p_upload ||
            (p_encoder->encode_buffer_max && NULL == p_encoder->p_buffer)) {
        PR_ERR("invalid params");
        return OPRT_INVALID_PARM;
    }

    if (!p_encoder->encode_buffer_max) {
        ret = tuya_voice_upload_send(p_upload->ctx, (uint8_t *)p_buffer, size);
        p_encoder->count ++;

#if defined(SPEAKER_TEST_STREAM_FILE) && (SPEAKER_TEST_STREAM_FILE == SPEAKER_TEST_ENCODE)
        speaker_write_tmp_file(p_encoder->file_fd, p_buffer, size);
#endif
    } else {
        if (size > p_encoder->encode_buffer_max) {
            PR_ERR("size:%d or upload buffer len:%d error", size, p_encoder->encode_buffer_max);
            return OPRT_INVALID_PARM;
        }

        p_encoder->encode_len += size;
        if (size + p_encoder->buffer_offset > p_encoder->encode_buffer_max) {
            ret = tuya_voice_upload_send(p_upload->ctx, (uint8_t *)p_encoder->p_buffer, p_encoder->buffer_offset);
            p_encoder->count ++;
            p_encoder->buffer_offset = 0;

#if defined(SPEAKER_TEST_STREAM_FILE) && (SPEAKER_TEST_STREAM_FILE == SPEAKER_TEST_ENCODE)
            speaker_write_tmp_file(p_encoder->file_fd, p_buffer, size);
#endif
        }

        memcpy(p_encoder->p_buffer + p_encoder->buffer_offset, p_buffer, size);
        p_encoder->buffer_offset += size;
    }

    return ret;
}

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
OPERATE_RET speaker_encode_free(MEDIA_UPLOAD_TASK_S *p_upload)
{
    OPERATE_RET ret = OPRT_OK;

    if (NULL == p_upload) {
        PR_ERR("invalid params");
        return OPRT_INVALID_PARM;
    }

    SPEAKER_MEDIA_ENCODER_S *p_encoder = &p_upload->encoder;
    if (p_encoder->buffer_offset) {
        ret = tuya_voice_upload_send(p_upload->ctx, (uint8_t *)p_encoder->p_buffer, p_encoder->buffer_offset);
        p_encoder->buffer_offset = 0;
    }

#if defined(SPEAKER_TEST_STREAM_FILE) && (SPEAKER_TEST_STREAM_FILE == SPEAKER_TEST_ENCODE)
    speaker_close_tmp_file(p_encoder->file_fd);
#endif

    p_encoder->encoder_free(&p_upload->encoder);

    return ret;
}

static SPEAKER_MEDIA_ENCODER_S *__find_encoder_by_type(TUYA_VOICE_AUDIO_FORMAT_E type)
{
    int i = 0;

    for (i = 0; i < s_speaker_encoder.encoder_cnt; i++) {
        if (type == s_speaker_encoder.encoder_arr[i].type
                && s_speaker_encoder.encoder_arr[i].encoder) {
            return s_speaker_encoder.encoder_arr[i].encoder;
        }
    }
    PR_ERR("don't find type %d valid encoder", type);
    return NULL;
}

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
OPERATE_RET speaker_encode_start(MEDIA_UPLOAD_TASK_S *p_upload, const SPEAKER_ENCODE_INFO_S *p_param)
{
    OPERATE_RET ret = OPRT_COM_ERROR;

    if (NULL == p_upload || NULL == p_param) {
        PR_ERR("invalid params");
        return OPRT_INVALID_PARM;
    }

    PR_DEBUG("encode_type:%d channels:%d rate:%d bits_per_sample:%d",
             p_param->encode_type, p_param->info.channels,
             p_param->info.rate, p_param->info.bits_per_sample);
    SPEAKER_MEDIA_ENCODER_S *p_encoder = &p_upload->encoder;
    memset(p_encoder, 0x0, sizeof(SPEAKER_MEDIA_ENCODER_S));

    SPEAKER_MEDIA_ENCODER_S *find = __find_encoder_by_type(p_param->encode_type);
    if (find) {
        memcpy(p_encoder, find, sizeof(SPEAKER_MEDIA_ENCODER_S));

        /** FIXME: internal filling some data & methods, but external call */
        p_encoder->encoder_data_callback = speaker_encode_result_write;
        memcpy(&p_encoder->param, p_param, sizeof(SPEAKER_ENCODE_INFO_S));

#if defined(SPEAKER_TEST_STREAM_FILE) && (SPEAKER_TEST_STREAM_FILE == SPEAKER_TEST_ENCODE)
        p_encoder->file_fd = speaker_create_tmp_file();
#endif

        if ((p_encoder->p_encode_info = p_encoder->encoder_init(p_encoder))) {
            return OPRT_OK;
        } else {
            PR_ERR("speaker encoder type %d %s init failed", p_param->encode_type, p_encoder->name);
        }
    }

    return ret;
}

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
OPERATE_RET speaker_encode_register_cb(SPEAKER_MEDIA_ENCODER_S *encoder)
{
    if (NULL == encoder) {
        PR_ERR("invalid params");
        return OPRT_INVALID_PARM;
    }

    if (s_speaker_encoder.encoder_cnt >= MAX_SPEAKER_ENCODER_NUM) {
        PR_ERR("cannot register cbs. reach max %d", s_speaker_encoder.encoder_cnt);
        return OPRT_INVALID_PARM;
    }

    SPEAKER_ENCODER_HANDLER_S *p_encoder_handler = NULL;
    p_encoder_handler = &s_speaker_encoder.encoder_arr[s_speaker_encoder.encoder_cnt];
    p_encoder_handler->type     = encoder->param.encode_type;
    p_encoder_handler->handler  = /* handler */ NULL;
    p_encoder_handler->encoder  = (SPEAKER_MEDIA_ENCODER_S *)calloc(1, sizeof(SPEAKER_MEDIA_ENCODER_S));
    memcpy(p_encoder_handler->encoder, encoder, sizeof(SPEAKER_MEDIA_ENCODER_S));
    s_speaker_encoder.encoder_cnt ++;
    PR_DEBUG("encoder_cnt: %d, type: %d, name: %s", s_speaker_encoder.encoder_cnt,
             p_encoder_handler->type, p_encoder_handler->encoder->name);

    return OPRT_OK;
}

/**
 * @brief Initialize the speaker encode module
 *
 * This function initializes the speaker encode module. Currently, it serves as a
 * placeholder for future initialization requirements.
 *
 * @return @ref OPERATE_RET
 * @retval OPRT_OK Always returns success
 */
OPERATE_RET speaker_encode_init(void)
{
    /** do nothing! */
    return OPRT_OK;
}
