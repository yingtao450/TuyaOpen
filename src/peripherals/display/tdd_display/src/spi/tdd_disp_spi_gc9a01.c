/**
 * @file tdd_disp_spi_gc9a01.c
 * @brief Implementation of GC9A01 TFT LCD driver with SPI interface. This file
 *        provides hardware-specific control functions for GC9A01 series TFT
 *        displays, including initialization sequence, pixel data transfer,
 *        and display control commands through SPI communication.
 *
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 *
 */
#include "tuya_cloud_types.h"
#include "tal_log.h"

#include "tdd_disp_gc9a01.h"
#include "tdl_display_driver.h"

/***********************************************************
***********************const define**********************
***********************************************************/
const uint8_t cGC9A01_INIT_SEQ[] = {
    1,    0,    0xEF, 
    2,    0,    0xEB, 0x14, 
    1,    0,    0xFE, 
    1,    0,    0xEF, 
    2,    0,    0xEB, 0x14,
    2,    0,    0x84, 0x40, 
    2,    0,    0x85, 0xFF, 
    2,    0,    0x86, 0xFF, 
    2,    0,    0x87, 0xFF, 
    2,    0,    0x88, 0x0A, 
    2,    0,    0x89, 0x21, 
    2,    0,    0x8A, 0x00, 
    2,    0,    0x8B, 0x80, 
    2,    0,    0x8C, 0x01, 
    2,    0,    0x8D, 0x01, 
    2,    0,    0x8E, 0xFF, 
    2,    0,    0x8F, 0xFF,
    3,    0,    0xB6, 0x00, 0x00, 
    2,    0,    0x36, 0x48, 
    2,    0,    0x3A, 0x05, //
    5,    0,    0x90, 0x08, 0x08, 0x08, 0x08,
    2,    0,    0xBD, 0x06,
    2,    0,    0xBC, 0x00, 
    4,    0,    0xFF, 0x60, 0x01, 0x04, 
    4,    0,    0xC3, 0x13, 0xC4, 0x13, 
    2,    0,    0xC9, 0x22, 
    2,    0,    0xBE, 0x11, 
    3,    0,    0xE1, 0x10, 0x0E, 
    4,    0,    0xDF, 0x31, 0x0c, 0x02,
    7,    0,    0xF0, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A, 
    7,    0,    0xF1, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F, 
    7,    0,    0xF2, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A, 
    7,    0,    0xF3, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F, 
    3,    0,    0xED, 0x1B, 0x0B,
    2,    0,    0xAE, 0x77,
    2,    0,    0xCD, 0x63,
    10,   0,    0x70, 0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03,
    2,    0,    0xE8, 0x34,
    13,   0,    0x62, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,
    13,   0,    0x63, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,
    8,    0,    0x64, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07,
    11,   0,    0x66, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00, 
    11,   0,    0x67, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98,
    8,    0,    0x74, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,
    3,    0,    0x98, 0x3e, 0x07,
    1,    0,    0x35, 1,    0,    0x21,
    1,    120,  0x11, 1,    20,   0x29,
    0 // Terminate list
};

static TDD_DISP_SPI_CFG_T sg_disp_rgb = {
    .cfg = {
        .cmd_caset = GC9A01_CASET,
        .cmd_raset = GC9A01_RASET,
        .cmd_ramwr = GC9A01_RAMWR,
    },
    
    .init_seq = cGC9A01_INIT_SEQ,
};

/***********************************************************
***********************function define**********************
***********************************************************/
OPERATE_RET tdd_disp_spi_gc9a01_register(char *name, DISP_SPI_DEVICE_CFG_T *dev_cfg)
{
    if (NULL == name || NULL == dev_cfg) {
        return OPRT_INVALID_PARM;
    }

    PR_NOTICE("tdd_disp_spi_gc9a01_register: %s", name);

    sg_disp_rgb.cfg.width     = dev_cfg->width;
    sg_disp_rgb.cfg.height    = dev_cfg->height;
    sg_disp_rgb.cfg.pixel_fmt = dev_cfg->pixel_fmt;
    sg_disp_rgb.cfg.port      = dev_cfg->port;
    sg_disp_rgb.cfg.spi_clk   = dev_cfg->spi_clk;
    sg_disp_rgb.cfg.cs_pin    = dev_cfg->cs_pin;
    sg_disp_rgb.cfg.dc_pin    = dev_cfg->dc_pin;
    sg_disp_rgb.cfg.rst_pin   = dev_cfg->rst_pin;

    memcpy(&sg_disp_rgb.power, &dev_cfg->power, sizeof(TUYA_DISPLAY_IO_CTRL_T));
    memcpy(&sg_disp_rgb.bl, &dev_cfg->bl, sizeof(TUYA_DISPLAY_BL_CTRL_T));

    return tdl_disp_spi_device_register(name, &sg_disp_rgb);
}