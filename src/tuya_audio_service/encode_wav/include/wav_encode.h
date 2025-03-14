/**
 * @file wav_encode.h
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

#ifndef WAV_ENCODE_H
#define WAV_ENCODE_H

#include "speaker_upload.h"
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#define WAV_ENCODE_BUFFER_LEN   (0)

#ifdef __cplusplus
extern "C" {
#endif

extern SPEAKER_MEDIA_ENCODER_S global_tuya_wav_encoder;

#ifdef __cplusplus
}
#endif

#endif /** !WAV_ENCODE_H */

