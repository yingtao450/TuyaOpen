#include "protobuf_utils.h"
#include "tal_api.h"
#include "tuya_list.h"
#include "mix_method.h"

static char* _itoa(int val, int radix, char* str)
{
    char tmp[33];
    int i, neg;
    char *tptr, *sptr;

    if (radix<2 || radix>36 || !str) {
        return NULL;
    }

    if (radix == 10 && val < 0 ) {
        neg = 1;
        val = -val;
    } else {
        neg = 0;
    }

    tptr = tmp;
    sptr = str;

    do {
        i = val % radix;
        if (i < 10)
            *tptr++ = (char)(i + '0');
        else
            *tptr++ = (char)(i + 'a' - 10);
    } while((val/=radix) > 0);

    if (neg)
        *sptr++ = '-';

    while(tptr > tmp)
        *sptr++ = *--tptr;

    *sptr = 0;

    return str;
}

OPERATE_RET pb_enc_opt_entry_init(PB_ENC_OPT_ENTRY_S *p_root, PB_ENC_OPT_ENTRY_INIT_CB init_cb)
{
    TUYA_CHECK_NULL_RETURN(p_root,OPRT_INVALID_PARM);
    TUYA_CHECK_NULL_RETURN(init_cb,OPRT_INVALID_PARM);

    p_root->init_cb = init_cb;
    p_root->node_num = 0;
    INIT_LIST_HEAD(&p_root->list_head);
    p_root->data_arr = NULL;

    return OPRT_OK;
}


OPERATE_RET pb_enc_opt_entry_set_kv_string(PB_ENC_OPT_ENTRY_S *root, char *key, char *val)
{
    TUYA_CHECK_NULL_RETURN(root,OPRT_INVALID_PARM);
    TUYA_CHECK_NULL_RETURN(key,OPRT_INVALID_PARM);
    TUYA_CHECK_NULL_RETURN(val,OPRT_INVALID_PARM);
    TUYA_CHECK_NULL_RETURN(root->init_cb,OPRT_INVALID_PARM);

    struct opt_entry_node *entry = Malloc(sizeof(struct opt_entry_node));
    if (entry == NULL) {
        PR_ERR("p_node Malloc faild");
        return OPRT_MALLOC_FAILED;
    }

    root->init_cb((struct opt_entry_data *)&entry->data);

    if ((entry->data.key = mm_strdup(key)) == NULL) {
        Free(entry);
        return OPRT_MALLOC_FAILED;
    }
    if ((entry->data.val = mm_strdup(val)) == NULL) {
        Free(entry->data.key);
        Free(entry);
        return OPRT_MALLOC_FAILED;
    }

    tuya_list_add_tail(&entry->node, &root->list_head);

    root->node_num = root->node_num + 1;

    return OPRT_OK;
}

OPERATE_RET pb_enc_opt_entry_set_kv_integer(PB_ENC_OPT_ENTRY_S *root, char *key, int val)
{
    char str[16] = {0};
    if (_itoa(val, 10, str) == NULL) {
        PR_ERR("itoa %d faild", val);
        return OPRT_COM_ERROR;
    }

    return pb_enc_opt_entry_set_kv_string(root, key, str);
}


OPERATE_RET pb_enc_opt_entry_destory(PB_ENC_OPT_ENTRY_S *p_root)
{
    struct tuya_list_head *pos = NULL;
    struct tuya_list_head *tmp = NULL;
    struct opt_entry_node *entry = NULL;
    TUYA_CHECK_NULL_RETURN(p_root,OPRT_INVALID_PARM);

    tuya_list_for_each_safe(pos, tmp, &p_root->list_head) {
        entry = tuya_list_entry(pos, struct opt_entry_node, node);
        if (entry != NULL) {
            tuya_list_del(&entry->node);
            if (entry->data.val) {
                Free(entry->data.val);
            }
            if (entry->data.key) {
                Free(entry->data.key);
            }
            Free(entry);
        }
    }

    if (p_root->data_arr != NULL)
        Free(p_root->data_arr);

    return OPRT_OK;
}

OPERATE_RET pb_enc_opt_entry_create_arr(PB_ENC_OPT_ENTRY_S *p_root)
{
    TUYA_CHECK_NULL_RETURN(p_root,OPRT_INVALID_PARM);
    if (p_root->node_num <= 0) {
        PR_ERR("protobuf option entry num is invalid");
        pb_enc_opt_entry_destory(p_root);
        return OPRT_COM_ERROR;
    }

    p_root->data_arr = Malloc(p_root->node_num * sizeof(struct opt_entry_data *));
    if (p_root->data_arr == NULL) {
        PR_ERR("p_root->output Malloc faild");
        pb_enc_opt_entry_destory(p_root);
        return OPRT_MALLOC_FAILED;
    }

    struct tuya_list_head *pos = NULL;
    struct opt_entry_node *entry = NULL;
    uint32_t i = 0;
    tuya_list_for_each(pos, &p_root->list_head){
        entry = tuya_list_entry(pos, struct opt_entry_node, node);
        p_root->data_arr[i++] = &entry->data;
    }

    return OPRT_OK;
}

