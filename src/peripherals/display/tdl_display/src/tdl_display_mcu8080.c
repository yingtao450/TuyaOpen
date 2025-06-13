/**
 * @file tdl_display_mcu8080.c
 * @version 0.1
 * @date 2025-05-27
 */

#include "tal_api.h"

#if defined(ENABLE_MCU8080) && (ENABLE_MCU8080==1)
#include "tkl_gpio.h"
#include "tkl_8080.h"
#include "tdl_display_manage.h"
#include "tdl_display_driver.h"
/***********************************************************
************************macro define************************
***********************************************************/

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    uint16_t                 width;
    uint16_t                 height;
    TUYA_DISPLAY_PIXEL_FMT_E fmt;
    SEM_HANDLE               tx_sem;
    SEM_HANDLE               te_sem;
    bool                     has_flushed_flag;
    bool                     flush_start_flag;
} TDL_DISP_8080_MANAGE_T;

typedef struct {
	TUYA_8080_BASE_CFG_T      cfg;
    TUYA_GPIO_NUM_E           te_pin;
    TUYA_GPIO_IRQ_E           te_mode;
    uint8_t                   cmd_caset;
    uint8_t                   cmd_raset;
    uint8_t                   cmd_ramwr;
    uint8_t                   cmd_ramwrc;
    const uint32_t           *init_seq;
}DISP_8080_DEV_T;

/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
static TDL_DISP_8080_MANAGE_T  sg_display_8080 = {0};

/***********************************************************
***********************function define**********************
***********************************************************/

static void __display_8080_isr(TUYA_MCU8080_EVENT_E event)
{
    tkl_8080_transfer_stop();
    if(TUYA_MCU8080_OUTPUT_FINISH ==event && sg_display_8080.tx_sem) {
       tal_semaphore_post(sg_display_8080.tx_sem); 
    }
}

void __te_isr_cb(void *args)
{
    if(sg_display_8080.te_sem && sg_display_8080.flush_start_flag) {
        tal_semaphore_post(sg_display_8080.te_sem);
    }
}

static OPERATE_RET __display_8080_gpio_init(DISP_8080_DEV_T *device)
{
    OPERATE_RET rt = OPRT_OK;
    TUYA_GPIO_IRQ_T irq_cfg;
    TUYA_GPIO_BASE_CFG_T gpio_cfg;

    if(device->te_pin < TUYA_GPIO_NUM_MAX) {
        gpio_cfg.direct = TUYA_GPIO_INPUT;

        if(device->te_mode == TUYA_GPIO_IRQ_RISE)
            gpio_cfg.mode = TUYA_GPIO_PULLDOWN;
        else if(device->te_mode == TUYA_GPIO_IRQ_FALL)
            gpio_cfg.mode = TUYA_GPIO_PULLUP;

        TUYA_CALL_ERR_RETURN(tkl_gpio_init(device->te_pin, &gpio_cfg));

        irq_cfg.mode = device->te_mode;
        irq_cfg.cb = __te_isr_cb;
        irq_cfg.arg = NULL;
        TUYA_CALL_ERR_RETURN(tkl_gpio_irq_init(device->te_pin, &irq_cfg));
        TUYA_CALL_ERR_RETURN(tkl_gpio_irq_enable(device->te_pin));

        PR_NOTICE("te_pin:%d, te_mode:%d", device->te_pin, device->te_mode);
    }

    return rt;
}

static OPERATE_RET __display_8080_gpio_deinit(DISP_8080_DEV_T *device)
{
    OPERATE_RET rt = OPRT_OK;

    if(device->te_pin < TUYA_GPIO_NUM_MAX) {
        TUYA_CALL_ERR_RETURN(tkl_gpio_irq_disable(device->te_pin));
        TUYA_CALL_ERR_RETURN(tkl_gpio_deinit(device->te_pin));
    }

    return rt;
}

static void __tdd_disp_init_seq(const uint32_t *init_seq)
{
	uint32_t *init_line = (uint32_t *)init_seq, *p_data = NULL;
    uint32_t data_len = 0, sleep_time = 0, cmd = 0;

    while (*init_line) {
        data_len   = init_line[0] - 1;
        sleep_time = init_line[1];
        cmd        = init_line[2];

        if(data_len) {
            p_data = &init_line[3];
        }else {
            p_data = NULL;
        }

        tkl_8080_cmd_send_with_param(cmd, p_data, (uint8_t)data_len);

        tal_system_sleep(sleep_time);
        init_line += init_line[0] + 2;
    }
}

static void __disp_8080_set_window(DISP_8080_DEV_T *p_cfg, uint32_t width, uint32_t height)
{
    uint32_t lcd_data[4];

    if(NULL == p_cfg) {
        return;
    }

    lcd_data[0] = 0;
    lcd_data[1] = 0;
    lcd_data[2] = (width >> 8) & 0xFF;
    lcd_data[3] = (width & 0xFF) - 1;
    tkl_8080_cmd_send_with_param(p_cfg->cmd_caset, lcd_data, 4);

    lcd_data[0] = 0;
    lcd_data[1] = 0;
    lcd_data[2] = (height >> 8) & 0xFF;
    lcd_data[3] = (height & 0xFF) - 1;
    tkl_8080_cmd_send_with_param(p_cfg->cmd_raset, lcd_data, 4);
}

static OPERATE_RET __tdd_display_mcu8080_open(TDD_DISP_DEV_HANDLE_T device)
{
    OPERATE_RET rt = OPRT_OK;
    DISP_8080_DEV_T *tdd_8080 = NULL;

    if(NULL == device) {
        return OPRT_INVALID_PARM;
    }
    tdd_8080 = (DISP_8080_DEV_T *)device;

    TUYA_CALL_ERR_RETURN(__display_8080_gpio_init(tdd_8080));

    TUYA_CALL_ERR_RETURN(tal_semaphore_create_init(&(sg_display_8080.tx_sem), 0, 1));
    TUYA_CALL_ERR_RETURN(tal_semaphore_create_init(&(sg_display_8080.te_sem), 0, 1));

    TUYA_CALL_ERR_RETURN(tkl_8080_init(&(tdd_8080->cfg)));

    __tdd_disp_init_seq(tdd_8080->init_seq);

    TUYA_CALL_ERR_RETURN(tkl_8080_irq_cb_register(__display_8080_isr));

    return rt;
}

static OPERATE_RET __tdd_display_mcu8080_flush(TDD_DISP_DEV_HANDLE_T device, TDL_DISP_FRAME_BUFF_T *frame_buff)
{
    OPERATE_RET rt = OPRT_OK;
    DISP_8080_DEV_T *tdd_8080 = NULL;

    if(NULL == device || NULL == frame_buff) {
        return OPRT_INVALID_PARM;
    }
    tdd_8080 = (DISP_8080_DEV_T *)device;

    if(sg_display_8080.width != frame_buff->width || sg_display_8080.height != frame_buff->height) {
        tkl_8080_ppi_set(frame_buff->width, frame_buff->height);
        sg_display_8080.width  = frame_buff->width;
        sg_display_8080.height = frame_buff->height;
    }

    if(sg_display_8080.fmt != frame_buff->fmt) {
        tkl_8080_pixel_mode_set(frame_buff->fmt);
        sg_display_8080.fmt = frame_buff->fmt;
    }

    tkl_8080_base_addr_set((uint32_t)frame_buff->frame);


    /*Wait for the TE interrupt to be given after a frame is completely scanned inside the screen, 
    *and then start sending data to rewrite the frame buffer of the screen to avoid screen display tearing. */
    /*If the module does not connect to the TE pin of the screen, the te_ipn should be set to TUYA_GPIO_NUM_MAX.*/
    if(tdd_8080->te_pin < TUYA_GPIO_NUM_MAX) {
        sg_display_8080.flush_start_flag = true;
        rt = tal_semaphore_wait(sg_display_8080.te_sem, 5000);
        sg_display_8080.flush_start_flag = false;
        if(rt) {
            PR_ERR("flush error(%d)...", rt);
            return rt;
        }
    }

    if(false == sg_display_8080.has_flushed_flag) {
        __disp_8080_set_window(tdd_8080, sg_display_8080.width, sg_display_8080.height);

        tkl_8080_cmd_send(tdd_8080->cmd_ramwr);
    }else {
        tkl_8080_cmd_send(tdd_8080->cmd_ramwrc);     
    }
    
    tkl_8080_transfer_start();

    return tal_semaphore_wait(sg_display_8080.tx_sem, SEM_WAIT_FOREVER);
}

static OPERATE_RET __tdd_display_mcu8080_close(TDD_DISP_DEV_HANDLE_T device)
{
    OPERATE_RET rt = OPRT_OK;
    DISP_8080_DEV_T *tdd_8080 = NULL;

    if(NULL == device) {
        return OPRT_INVALID_PARM;
    }
    tdd_8080 = (DISP_8080_DEV_T *)device;

    TUYA_CALL_ERR_RETURN(tkl_8080_deinit());

    TUYA_CALL_ERR_RETURN(__display_8080_gpio_deinit(tdd_8080));

    //sem deinit
    rt |= tal_semaphore_release(sg_display_8080.tx_sem);
    sg_display_8080.tx_sem = NULL;
    rt |= tal_semaphore_release(sg_display_8080.te_sem);
    sg_display_8080.te_sem = NULL;

    sg_display_8080.has_flushed_flag = 0;
    sg_display_8080.flush_start_flag = 0;
    sg_display_8080.fmt = 0xff;
    sg_display_8080.width = 0;
    sg_display_8080.height = 0;

    return rt;
}

OPERATE_RET tdl_disp_mcu8080_device_register(char *name, TDD_DISP_MCU8080_CFG_T *mcu8080)
{
    OPERATE_RET rt = OPRT_OK;
    DISP_8080_DEV_T *tdd_8080 = NULL;
    TDD_DISP_DEV_INFO_T mcu8080_dev_info;

    if(NULL == name || NULL == mcu8080) {
        return OPRT_INVALID_PARM;
    }

    tdd_8080 = tal_malloc(sizeof(DISP_8080_DEV_T));
    if(NULL == tdd_8080) {
        return OPRT_MALLOC_FAILED;
    }
    memcpy(&tdd_8080->cfg, &mcu8080->cfg, sizeof(TUYA_8080_BASE_CFG_T));

    tdd_8080->init_seq   = mcu8080->init_seq;
    tdd_8080->te_pin     = mcu8080->te_pin;
    tdd_8080->te_mode    = mcu8080->te_mode;
    tdd_8080->cmd_caset  = mcu8080->cmd_caset;
    tdd_8080->cmd_raset  = mcu8080->cmd_raset;
    tdd_8080->cmd_ramwr  = mcu8080->cmd_ramwr;
    tdd_8080->cmd_ramwrc = mcu8080->cmd_ramwrc;

    mcu8080_dev_info.type       = TUYA_DISPLAY_8080;
    mcu8080_dev_info.width      = mcu8080->cfg.width;
    mcu8080_dev_info.height     = mcu8080->cfg.height;
    mcu8080_dev_info.fmt        = mcu8080->cfg.pixel_fmt;
    mcu8080_dev_info.rotation   = mcu8080->rotation;

    memcpy(&mcu8080_dev_info.bl, &mcu8080->bl, sizeof(TUYA_DISPLAY_BL_CTRL_T));
    memcpy(&mcu8080_dev_info.power, &mcu8080->power, sizeof(TUYA_DISPLAY_IO_CTRL_T));

    TDD_DISP_INTFS_T mcu8080_intfs = {
        .open  = __tdd_display_mcu8080_open,
        .flush = __tdd_display_mcu8080_flush,
        .close = __tdd_display_mcu8080_close,
    };

    TUYA_CALL_ERR_RETURN(tdl_disp_device_register(name, (TDD_DISP_DEV_HANDLE_T)tdd_8080,\
                                                  &mcu8080_intfs, &mcu8080_dev_info));

    return OPRT_OK;
}

#endif