
/**
 * @file speaker_encode_types.h
 * @brief Defines the speaker encoder type definitions for Tuya's audio service.
 *
 * This header file provides the core type definitions and structures for
 * speaker audio encoding within the Tuya audio service framework. It includes
 * the declaration of encoder structures, callback function types, and parameter
 * definitions used in audio encoding operations. The types defined in this file
 * support various encoding formats and configurations, enabling flexible and
 * efficient audio data processing. This file is essential for developers
 * implementing custom audio encoders or working with Tuya's audio encoding
 * subsystem.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __SPEAKER_ENCODE_TYPES_H__
#define __SPEAKER_ENCODE_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "speaker_upload.h"

struct speaker_media_encoder {
    void                   *handle;             /*< self object instance point */
    char                 *name;               /*< object instance name */

    uint32_t                  encode_buffer_max;  /*< encode buffer length max | encode_buffer_max */
    void                   *p_encode_info;      /*< encode context | encode_ctx */
    uint8_t                 *p_start_data;       /*< encode start head data | head_data */
    uint32_t                  start_data_len;     /*< encode start head data length | head_data_len */
    uint32_t                  encode_len;         /*< encode buffer length | encode_buffer_len */
    uint8_t                 *p_buffer;           /*< encode buffer | encode_buffer */
    uint32_t                  buffer_offset;      /*< encode buffer offset | encode_buffer_offset */
    SPEAKER_ENCODE_INFO_S   param;              /*< encode parameters | param */
    uint32_t                  count;              /*< encode real length count | count */
    int                   file_fd;            /*< encode debug dump file fd */
    void       *(*encoder_init)(struct speaker_media_encoder *encoder);
    OPERATE_RET (*encoder_deinit)(struct speaker_media_encoder *encoder);
    uint32_t      (*encoder_encode)(struct speaker_media_encoder *encoder, void *private_data, const uint8_t *buffer, uint32_t size);
    OPERATE_RET (*encoder_free)(struct speaker_media_encoder *encoder);
    OPERATE_RET (*encoder_data_callback)(struct speaker_media_encoder *encoder, void *private_data, const uint8_t *buffer, uint32_t size);
};

#ifdef __cplusplus
}
#endif

#endif /** !__SPEAKER_ENCODE_TYPES_H__ */
