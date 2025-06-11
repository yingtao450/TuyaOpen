/**
 * @file tdl_display_driver.h

 * @version 0.1
 * @date 2025-05-27
 */

#ifndef __TDL_DISPLAY_DRIVER_H__
#define __TDL_DISPLAY_DRIVER_H__

#include "tuya_cloud_types.h"
#include "tdl_display_manage.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/
#define DISPLAY_DEV_NAME_MAX_LEN 32

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef void*  TDD_DISP_DEV_HANDLE_T;

typedef OPERATE_RET (*TDD_DISPLAY_SEQ_INIT_CB)(void);

typedef struct {
    TUYA_DISPLAY_TYPE_E         type;
    uint16_t                    width;
    uint16_t                    height;
    TUYA_DISPLAY_PIXEL_FMT_E    fmt;
    TUYA_DISPLAY_BL_CTRL_T      bl;
    TUYA_DISPLAY_IO_CTRL_T      power;
}TDD_DISP_DEV_INFO_T;

typedef struct {
    TUYA_RGB_BASE_CFG_T         cfg;
    TUYA_DISPLAY_BL_CTRL_T      bl;
    TUYA_DISPLAY_IO_CTRL_T      power;
    TDD_DISPLAY_SEQ_INIT_CB     init_cb; 
}TDD_DISP_RGB_CFG_T;

typedef struct {
    uint16_t                    width;
    uint16_t                    height;
    TUYA_DISPLAY_PIXEL_FMT_E    pixel_fmt;
    TUYA_GPIO_NUM_E             cs_pin;
    TUYA_GPIO_NUM_E             dc_pin;
    TUYA_GPIO_NUM_E             rst_pin;
    TUYA_SPI_NUM_E              port;
    uint32_t                    spi_clk;
    uint8_t                     cmd_caset;
    uint8_t                     cmd_raset;
    uint8_t                     cmd_ramwr;
}DISP_SPI_BASE_CFG_T;

typedef struct {
    uint16_t                    width;
    uint16_t                    height;
    TUYA_DISPLAY_PIXEL_FMT_E    pixel_fmt;
    TUYA_GPIO_NUM_E             cs_pin;
    TUYA_GPIO_NUM_E             dc_pin;
    TUYA_GPIO_NUM_E             rst_pin;
    TUYA_QSPI_NUM_E             port;
    uint32_t                    spi_clk;
    uint8_t                     cmd_caset;
    uint8_t                     cmd_raset;
    uint8_t                     cmd_ramwr;
}DISP_QSPI_BASE_CFG_T;

typedef struct { 
    DISP_SPI_BASE_CFG_T         cfg;
    TUYA_DISPLAY_BL_CTRL_T      bl;
    TUYA_DISPLAY_IO_CTRL_T      power;
    const uint8_t              *init_seq; // Initialization commands for the display
}TDD_DISP_SPI_CFG_T;

typedef struct { 
    DISP_QSPI_BASE_CFG_T        cfg;
    TUYA_DISPLAY_BL_CTRL_T      bl;
    TUYA_DISPLAY_IO_CTRL_T      power;
    const uint8_t              *init_seq; // Initialization commands for the display
}TDD_DISP_QSPI_CFG_T;

typedef struct {
    OPERATE_RET (*open)(TDD_DISP_DEV_HANDLE_T  device);
    OPERATE_RET (*flush)(TDD_DISP_DEV_HANDLE_T device, TDL_DISP_FRAME_BUFF_T *frame_buff);     
    OPERATE_RET (*close)(TDD_DISP_DEV_HANDLE_T device);  
}TDD_DISP_INTFS_T;

/***********************************************************
********************function declaration********************
***********************************************************/
OPERATE_RET tdl_disp_device_register(char *name, TDD_DISP_DEV_HANDLE_T tdd_hdl, \
                                     TDD_DISP_INTFS_T *intfs, TDD_DISP_DEV_INFO_T *dev_info);

#if defined(ENABLE_RGB) && (ENABLE_RGB==1)                                     
OPERATE_RET tdl_disp_rgb_device_register(char *name, TDD_DISP_RGB_CFG_T *rgb);
#endif

#if defined(ENABLE_SPI) && (ENABLE_SPI==1)   
OPERATE_RET tdl_disp_spi_device_register(char *name, TDD_DISP_SPI_CFG_T *spi);
#endif

#if defined(ENABLE_QSPI) && (ENABLE_QSPI==1)   
OPERATE_RET tdl_disp_qspi_device_register(char *name, TDD_DISP_QSPI_CFG_T *qspi);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TDL_DISPLAY_DRIVER_H__ */
