/**
 * @file tdl_display_rgb.c
 * @brief tdl_display_rgb module is used to control RGB display
 * @version 0.1
 * @date 2025-05-27
 */

#include "tal_api.h"

#if defined(ENABLE_RGB) && (ENABLE_RGB==1)

#include "tkl_rgb.h"
#include "tdl_display_manage.h"
#include "tdl_display_driver.h"
/***********************************************************
************************macro define************************
***********************************************************/
#define UNACTIVE_LEVEL(level)  ((level == 1) ? 0 : 1)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    TDL_RGB_FRAME_REQUEST = 0,
    TDL_RGB_FRAME_EXIT,
}TDL_RGB_FRAME_EVENT_E;

typedef struct {
	TDL_RGB_FRAME_EVENT_E event;
	uint32_t              param;
}TDL_DISP_RGB_MSG_T;

typedef struct {
	uint8_t                is_task_running;
	TDL_DISP_FRAME_BUFF_T *pingpong_frame;
	TDL_DISP_FRAME_BUFF_T *display_frame;
	SEM_HANDLE             flush_sem;
	SEM_HANDLE             task_sem;
    MUTEX_HANDLE           mutex;
	THREAD_HANDLE          task;
	QUEUE_HANDLE           queue;
}TDL_DISP_RGB_INFO_T;

typedef struct {
	TUYA_RGB_BASE_CFG_T       cfg;
	TDD_DISPLAY_SEQ_INIT_CB   init_cb;
}DISP_RGB_DEV_T;

/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
static TDL_DISP_RGB_INFO_T  sg_display_rgb = {0};

/***********************************************************
***********************function define**********************
***********************************************************/

static void __display_rgb_isr(TUYA_RGB_EVENT_E event)
{
	if (sg_display_rgb.pingpong_frame != NULL) {
		if (sg_display_rgb.display_frame != NULL) {
			if (sg_display_rgb.pingpong_frame != sg_display_rgb.display_frame) {
				if (sg_display_rgb.display_frame->width != sg_display_rgb.pingpong_frame->width ||
                    sg_display_rgb.display_frame->height != sg_display_rgb.pingpong_frame->height) {
                    tkl_rgb_ppi_set(sg_display_rgb.pingpong_frame->width, sg_display_rgb.pingpong_frame->height);
				}

				if (sg_display_rgb.display_frame->fmt != sg_display_rgb.pingpong_frame->fmt) {
					tkl_rgb_pixel_mode_set(sg_display_rgb.pingpong_frame->fmt);
				}

                if (sg_display_rgb.display_frame != NULL &&
                    sg_display_rgb.display_frame->free_cb != NULL) {
                    sg_display_rgb.display_frame->free_cb(sg_display_rgb.display_frame);
                }
			}
			sg_display_rgb.display_frame = sg_display_rgb.pingpong_frame;
			sg_display_rgb.pingpong_frame = NULL;
			tkl_rgb_base_addr_set((uint32_t)sg_display_rgb.display_frame->frame);

			tal_semaphore_post(sg_display_rgb.flush_sem);
		}else {
			sg_display_rgb.display_frame = sg_display_rgb.pingpong_frame;
			sg_display_rgb.pingpong_frame = NULL;
			tal_semaphore_post(sg_display_rgb.flush_sem);
		}
	}
}

static OPERATE_RET __rgb_display_frame(TDL_DISP_FRAME_BUFF_T *frame)
{
	OPERATE_RET ret = 0;
	if (sg_display_rgb.display_frame == NULL) {
		tkl_rgb_ppi_set(frame->width, frame->height);
		tkl_rgb_pixel_mode_set(frame->fmt);
		sg_display_rgb.pingpong_frame = frame;

		tkl_rgb_base_addr_set((uint32_t)frame->frame);
		tkl_rgb_display_transfer_start();
	}else {
		if (sg_display_rgb.pingpong_frame != NULL) {
			PR_ERR("pingpong_frame can't be !NULL");
		}

		sg_display_rgb.pingpong_frame = frame;
	}

	ret = tal_semaphore_wait(sg_display_rgb.flush_sem, SEM_WAIT_FOREVER);
	if (ret != 0){
		PR_DEBUG("%s semaphore get failed: %d", __func__, ret);
	}

	return ret;
}

static void __rgb_task(void *arg)
{
    TDL_DISP_RGB_MSG_T msg = {0};
    OPERATE_RET ret = 0;

    sg_display_rgb.is_task_running = 1;

    while(sg_display_rgb.is_task_running) {
        ret = tal_queue_fetch(sg_display_rgb.queue, &msg, SEM_WAIT_FOREVER);
        if(ret == OPRT_OK) {
            switch(msg.event) {
                case TDL_RGB_FRAME_REQUEST:
                    __rgb_display_frame((TDL_DISP_FRAME_BUFF_T *)msg.param);
                    break;

                case TDL_RGB_FRAME_EXIT:
                    sg_display_rgb.is_task_running = false;
                    do {
                        ret = tal_queue_fetch(sg_display_rgb.queue, &msg, 0);//no wait
                        if(msg.event == TDL_RGB_FRAME_REQUEST) {
                            TDL_DISP_FRAME_BUFF_T *frame = (TDL_DISP_FRAME_BUFF_T *)msg.param;
                            if(frame && frame->free_cb) {
                                frame->free_cb(frame);
                            }
                        }
                    } while(ret == 0);
                    break;

                default:
                    break;

            }
        }
    }

    tal_semaphore_post(sg_display_rgb.task_sem);
    sg_display_rgb.task_sem = NULL;
    THREAD_HANDLE tmp_task = sg_display_rgb.task;
    sg_display_rgb.task = NULL;
    tal_thread_delete(tmp_task);

    return;
}
static OPERATE_RET __tdd_display_rgb_open(TDD_DISP_DEV_HANDLE_T device)
{
    OPERATE_RET rt = 0;
    DISP_RGB_DEV_T *tdd_rgb = NULL;
    TUYA_RGB_BASE_CFG_T *rgb_cfg = NULL;

    if(NULL == device) {
        return OPRT_INVALID_PARM;
    }

    tdd_rgb = (DISP_RGB_DEV_T *)device;

    rgb_cfg = &tdd_rgb->cfg;

    TUYA_CALL_ERR_RETURN(tal_semaphore_create_init(&(sg_display_rgb.flush_sem), 0, 1));
    TUYA_CALL_ERR_RETURN(tal_semaphore_create_init(&(sg_display_rgb.task_sem), 0, 1));
    TUYA_CALL_ERR_RETURN(tal_queue_create_init(&(sg_display_rgb.queue), sizeof(TDL_DISP_RGB_MSG_T), 32));
    TUYA_CALL_ERR_RETURN(tal_mutex_create_init(&sg_display_rgb.mutex));

    THREAD_CFG_T thread_cfg = {4096, THREAD_PRIO_1, "rgb_task"};
    TUYA_CALL_ERR_RETURN(tal_thread_create_and_start(&(sg_display_rgb.task), NULL, NULL, __rgb_task, NULL, &thread_cfg));

    if(tdd_rgb->init_cb) {
        tdd_rgb->init_cb();
    }

    PR_NOTICE("clk:%d", rgb_cfg->clk);

    TUYA_CALL_ERR_RETURN(tkl_rgb_init(rgb_cfg));

    TUYA_CALL_ERR_RETURN(tkl_rgb_irq_cb_register(__display_rgb_isr));

    return OPRT_OK;
}

static OPERATE_RET __tdd_display_rgb_flush(TDD_DISP_DEV_HANDLE_T device, TDL_DISP_FRAME_BUFF_T *frame_buff)
{
    if(NULL == device || NULL == frame_buff) {
        return OPRT_INVALID_PARM;
    }

    tal_mutex_lock(sg_display_rgb.mutex);

    if(sg_display_rgb.is_task_running) {
        TDL_DISP_RGB_MSG_T msg = {TDL_RGB_FRAME_REQUEST, (uint32_t)frame_buff};
        tal_queue_post(sg_display_rgb.queue, &msg , SEM_WAIT_FOREVER);
    }

    tal_mutex_unlock(sg_display_rgb.mutex);

    return OPRT_OK;
}

static OPERATE_RET __tdd_display_rgb_close(TDD_DISP_DEV_HANDLE_T device)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tdl_disp_rgb_device_register(char *name, TDD_DISP_RGB_CFG_T *rgb)
{
    OPERATE_RET rt = OPRT_OK;
    DISP_RGB_DEV_T *tdd_rgb = NULL;
    TDD_DISP_DEV_INFO_T rgb_dev_info;

    if(NULL == name || NULL == rgb) {
        return OPRT_INVALID_PARM;
    }

    tdd_rgb = tal_malloc(sizeof(DISP_RGB_DEV_T));
    if(NULL == tdd_rgb) {
        return OPRT_MALLOC_FAILED;
    }
    memcpy(&tdd_rgb->cfg, &rgb->cfg, sizeof(TUYA_RGB_BASE_CFG_T));

    tdd_rgb->init_cb = rgb->init_cb;

    rgb_dev_info.type   = TUYA_DISPLAY_RGB;
    rgb_dev_info.width  = rgb->cfg.width;
    rgb_dev_info.height = rgb->cfg.height;
    rgb_dev_info.fmt    = rgb->cfg.pixel_fmt;

    memcpy(&rgb_dev_info.bl, &rgb->bl, sizeof(TUYA_DISPLAY_BL_CTRL_T));
    memcpy(&rgb_dev_info.power, &rgb->power, sizeof(TUYA_DISPLAY_IO_CTRL_T));

    TDD_DISP_INTFS_T rgb_intfs = {
        .open  = __tdd_display_rgb_open,
        .flush = __tdd_display_rgb_flush,
        .close = __tdd_display_rgb_close,
    };

    PR_NOTICE("clk:%d", tdd_rgb->cfg.clk);

    TUYA_CALL_ERR_RETURN(tdl_disp_device_register(name, (TDD_DISP_DEV_HANDLE_T)tdd_rgb,\
                                                  &rgb_intfs, &rgb_dev_info));

    return OPRT_OK;
}

#endif