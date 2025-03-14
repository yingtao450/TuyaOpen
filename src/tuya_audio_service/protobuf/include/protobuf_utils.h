/**
 * @file protobuf_utils.h
 * @brief protobuf utilities
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */
#ifndef __PROTOBUF_UTILS_H__
#define __PROTOBUF_UTILS_H__

#include "tuya_cloud_types.h"
#include "protobuf-c/protobuf-c.h"
#include "tuya_list.h"

struct opt_entry_data{
  ProtobufCMessage base;
  char *key;
  char *val;
};

struct opt_entry_node{
    struct tuya_list_head node;
    struct opt_entry_data data;
};

typedef void (*PB_ENC_OPT_ENTRY_INIT_CB)(void *data);

typedef struct {
    PB_ENC_OPT_ENTRY_INIT_CB init_cb;
    uint32_t node_num;
    struct tuya_list_head list_head;
    struct opt_entry_data **data_arr;
} PB_ENC_OPT_ENTRY_S;

OPERATE_RET pb_enc_opt_entry_init(PB_ENC_OPT_ENTRY_S *p_root, PB_ENC_OPT_ENTRY_INIT_CB init_cb);
OPERATE_RET pb_enc_opt_entry_set_kv_string(PB_ENC_OPT_ENTRY_S *root, char *key, char *val);
OPERATE_RET pb_enc_opt_entry_set_kv_integer(PB_ENC_OPT_ENTRY_S *root, char *key, int val);
OPERATE_RET pb_enc_opt_entry_create_arr(PB_ENC_OPT_ENTRY_S *p_root);
OPERATE_RET pb_enc_opt_entry_destory(PB_ENC_OPT_ENTRY_S *p_root);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
