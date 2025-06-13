/**
 * @file tdd_disp_mcu8080_st7789.c
 * @brief Implementation of ST7789 TFT LCD driver with SPI interface. This file
 *        provides hardware-specific control functions for ST7789 series TFT
 *        displays, including initialization sequence, pixel data transfer,
 *        and display control commands through SPI communication.
 *
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 *
 */
#include "tuya_cloud_types.h"
#include "tal_log.h"

#include "tdd_disp_st7796s.h"
#include "tdl_display_driver.h"

/***********************************************************
***********************const define**********************
***********************************************************/
const uint8_t cST7796S_INIT_SEQ[] = {
    1,    0,    0x01, 
    1,    120,  0x28, 
    2,    0,    0xF0, 0xC3,
    2,    0,    0xF0, 0x96,
    2,    0,    0x35, 0x00,
    3,    0,    0x44, 0x00, 0x01,
    3,    0,    0xb1, 0x60, 0x11,  
    2,    0,    0x36, 0x98,
    2,    0,    0x3A, 0x55,
    2,    0,    0xB4, 0x01,
    2,    0,    0xB7, 0xC6,
    9,    0,    0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xa5, 0x33,
    2,    0,    0xC2, 0xA7,
    2,    0,    0xC5, 0x2B,
    15,   0,    0xE0, 0xF0, 0x09, 0x13, 0x12, 0x12, 0x2B, 0x3C, 0x44, 0x4B, 0x1B, 0x18, 0x17, 0x1D, 0x21,
    15,   0,    0xE1, 0xF0, 0x09, 0x13, 0x0C, 0x0D, 0x27, 0x3B, 0x44, 0x4D, 0x0B, 0x17, 0x17, 0x1D, 0x21,
    2,    0,    0xF0, 0x3C,
    2,    0,    0xF0, 0x96,
    1,    150,    0x11, 
    1,    0,    0x29, 
    0 // Terminate list
};

static TDD_DISP_MCU8080_CFG_T sg_disp_mcu8080_cfg = {
    .cmd_caset = ST7796S_CASET,
    .cmd_raset = ST7796S_RASET,
    .cmd_ramwr = ST7796S_RAMWR,
    .cmd_ramwr = ST7796S_RAMWRC,
    .init_seq = cST7796S_INIT_SEQ,
};

/***********************************************************
***********************function define**********************
***********************************************************/
OPERATE_RET tdd_disp_mcu8080_st7796s_register(char *name, DISP_MCU8080_DEVICE_CFG_T *dev_cfg)
{
    if (NULL == name || NULL == dev_cfg) {
        return OPRT_INVALID_PARM;
    }

    PR_NOTICE("tdd_disp_mcu8080_st7796s_register: %s", name);

    sg_disp_mcu8080_cfg.cfg.width     = dev_cfg->width;
    sg_disp_mcu8080_cfg.cfg.height    = dev_cfg->height;
    sg_disp_mcu8080_cfg.cfg.pixel_fmt = dev_cfg->pixel_fmt;
    sg_disp_mcu8080_cfg.cfg.clk       = dev_cfg->clk;
    sg_disp_mcu8080_cfg.cfg.data_bits = dev_cfg->data_bits;

    sg_disp_mcu8080_cfg.rotation  = dev_cfg->rotation;
    sg_disp_mcu8080_cfg.te_pin  = dev_cfg->te_pin;
    sg_disp_mcu8080_cfg.te_mode = dev_cfg->te_mode;

    memcpy(&sg_disp_mcu8080_cfg.power, &dev_cfg->power, sizeof(TUYA_DISPLAY_IO_CTRL_T));
    memcpy(&sg_disp_mcu8080_cfg.bl, &dev_cfg->bl, sizeof(TUYA_DISPLAY_BL_CTRL_T));

    return tdl_disp_mcu8080_device_register(name, &sg_disp_mcu8080_cfg);
}
