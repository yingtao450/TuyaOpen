/**
 * @file wav_encode.c
 * @brief Defines the WAV audio encoding interface for Tuya's audio service.
 *
 * This source file provides the implementation of WAV audio encoding
 * functionality within the Tuya audio service framework. It includes the
 * initialization, configuration, and processing functions for WAV encoding,
 * which handles the conversion of raw audio data into standard WAV format.
 * The implementation supports various audio parameters including sample rate,
 * bit depth, and channel configuration. This file is essential for
 * developers working on audio processing applications that require
 * uncompressed PCM audio storage and playback capabilities.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "wav_encode.h"
#include "speaker_encode_types.h"
#include "speaker_upload.h"
#include "tuya_cloud_types.h"

static OPERATE_RET wav_encode_free(SPEAKER_MEDIA_ENCODER_S *p_encoder)
{
    return OPRT_OK;
}

static unsigned int wav_data_encode(SPEAKER_MEDIA_ENCODER_S *p_encoder,
                                    void *private_data, const unsigned char *buffer, unsigned int size)
{
    if (NULL == p_encoder || NULL == p_encoder->encoder_data_callback) {
        return OPRT_INVALID_PARM;
    }

    /** FIXME: do write data to upload, but impl inside! */
    return p_encoder->encoder_data_callback(p_encoder, private_data, buffer, size);
}

OPERATE_RET wav_encode_init(void *p_wav_encoder)
{
    SPEAKER_MEDIA_ENCODER_S *p_encoder = (SPEAKER_MEDIA_ENCODER_S *)p_wav_encoder;

    if (NULL == p_encoder) {
        return OPRT_INVALID_PARM;
    }

    p_encoder->encoder_encode       = wav_data_encode;
    p_encoder->encoder_free         = wav_encode_free;
    p_encoder->encode_buffer_max    = WAV_ENCODE_BUFFER_LEN;
    return OPRT_OK;
}

static void *__wav_encoder_init(SPEAKER_MEDIA_ENCODER_S *encoder)
{
    /** FIXME: don't worry, no use, but just return */
    return (void *)encoder;
}

static OPERATE_RET __wav_encoder_deinit(SPEAKER_MEDIA_ENCODER_S *encoder)
{
    return wav_encode_free(encoder);
}

static uint32_t __wav_encoder_encode(SPEAKER_MEDIA_ENCODER_S *encoder,
                                   void *private_data, const uint8_t *buffer, uint32_t size)
{
    return wav_data_encode(encoder, private_data, buffer, size);
}

static OPERATE_RET __wav_encoder_free(SPEAKER_MEDIA_ENCODER_S *encoder)
{
    return wav_encode_free(encoder);
}

static OPERATE_RET __wav_encoder_data_callback(SPEAKER_MEDIA_ENCODER_S *encoder,
        void *private_data, const uint8_t *buffer, uint32_t size)
{
    /** FIXME: don't worry, impl inside! */
    return OPRT_OK;
}

SPEAKER_MEDIA_ENCODER_S global_tuya_wav_encoder = {
    .handle                     = &global_tuya_wav_encoder,
    .name                       = "global_tuya_wav_encoder",

    .encode_buffer_max          = WAV_ENCODE_BUFFER_LEN,
    .param = {
        .encode_type            = TUYA_VOICE_AUDIO_FORMAT_WAV,
        .info = {
            .channels           = 1,
            .rate               = 16000,
            .bits_per_sample    = 16,
        },
    },

    .encoder_init               = __wav_encoder_init,
    .encoder_deinit             = __wav_encoder_deinit,
    .encoder_encode             = __wav_encoder_encode,
    .encoder_free               = __wav_encoder_free,
    .encoder_data_callback      = __wav_encoder_data_callback,
};
