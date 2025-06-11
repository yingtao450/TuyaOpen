/**
 * @file tdd_touch_gt911.h
 * @version 0.1
 * @date 2025-06-09
 */

#ifndef __TDD_TOUCH_GT911_H__
#define __TDD_TOUCH_GT911_H__

#include "tuya_cloud_types.h"
#include "tdd_touch_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/
#define GT911_CUSTOM_CFG           (1)
#define GT911_I2C_SLAVE_ADDR       (0x28 >> 1)
#define GT911_STATUS               (0x814E)
#define GT911_POINT1_REG           (0x814F)
#define GT911_POINT2_REG           (0x8157)
#define GT911_POINT3_REG           (0x815F)
#define GT911_POINT4_REG           (0x8167)
#define GT911_POINT5_REG           (0x816F)
#define GT911_CONFIG_REG           (0x8047)
#define GT911_COMMAND_REG          (0x8040)
#define GT911_PRODUCT_ID_REG       (0x8140)
#define GT911_VENDOR_ID_REG        (0x814A)
#define GT911_CONFIG_VERSION_REG   (0x8047)
#define GT911_CONFIG_CHECKSUM_REG  (0x80FF)
#define GT911_FIRMWARE_VERSION_REG (0x8144)
#define GT911_X_RESOLUTION         (0x8146)
#define GT911_Y_RESOLUTION         (0x8148)

/* Max detectable simultaneous touch points */
#define GT911_I2C_MAX_POINT         (5)
#define GT911_POINT_INFO_SIZE       (8)
#define GT911_POINT_INFO_TOTAL_SIZE (GT911_I2C_MAX_POINT * GT911_POINT_INFO_SIZE)
/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/
OPERATE_RET tdd_touch_i2c_gt911_register(char *name, TDD_TOUCH_I2C_CFG_T *cfg);

#ifdef __cplusplus
}
#endif

#endif /* __TDD_TOUCH_GT911_H__ */
