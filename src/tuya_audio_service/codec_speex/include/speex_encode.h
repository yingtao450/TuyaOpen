/**
 * @file speex_encode.h
 * @brief Defines the Speex audio encoding interface for Tuya's audio service.
 *
 * This source file provides the implementation of Speex audio encoding
 * functionality within the Tuya audio service framework. It includes the
 * initialization, configuration, and processing functions for Speex encoding,
 * which handles the compression of audio data using the Speex codec. The
 * implementation supports various encoding parameters including quality settings,
 * bit rates, and frame sizes. This file is essential for developers working on
 * IoT applications that require efficient voice compression and transmission
 * while maintaining acceptable audio quality.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef SPEEX_ENCODE_H
#define SPEEX_ENCODE_H

#include "speaker_upload.h"
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SPEAKER_MEDIA_ENCODER_S global_tuya_speex_encoder;

#ifdef __cplusplus
}
#endif

#endif /** !SPEEX_ENCODE_H */
