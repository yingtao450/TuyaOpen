/**
 * @file tdd_touch_gt1151.h
 * @version 0.1
 * @date 2025-06-09
 */

#ifndef __TDD_TOUCH_GT1151_H__
#define __TDD_TOUCH_GT1151_H__

#include "tuya_cloud_types.h"
#include "tdd_touch_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/
#define GT1151_I2C_SLAVE_ADDR  (0x28 >> 1)
#define GT1151_PRODUCT_ID_CODE (0x38353131) // "1151" ASCII code

#define GT1151_ADDR_LEN      (1)
#define GT1151_REG_LEN       (2)
#define GT1151_MAX_TOUCH_NUM (5)

#ifndef TOUCH_SUPPORT_MAX_NUM
#define GT1151_POINT_INFO_NUM (1)
#else
#define GT1151_POINT_INFO_NUM (TOUCH_SUPPORT_MAX_NUM)
#endif
#define GT1151_POINT_INFO_SIZE       (8)
#define GT1151_POINT_INFO_TOTAL_SIZE (GT1151_POINT_INFO_NUM * GT1151_POINT_INFO_SIZE)

#define GT1151_COMMAND_REG (0x8040)

#define GT1151_CONFIG_REG (0x8050)

#define GT1151_PRODUCT_ID       (0x8140)
#define GT1151_FIRMWARE_VERSION (0x8144)
#define GT1151_VENDOR_ID        (0x814A)

#define GT1151_STATUS (0x814E)

#define GT1151_POINT1_REG (0x814F)
#define GT1151_POINT2_REG (0x8157)
#define GT1151_POINT3_REG (0x815F)
#define GT1151_POINT4_REG (0x8167)
#define GT1151_POINT5_REG (0x816F)

#define GT1151_CHECK_SUM (0x813C)

// config parameters and relative position or range.
#define GT1151_X_OUTPUT_MAX_POS   (1)
#define GT1151_Y_OUTPUT_MAX_POS   (3)
#define GT1151_TOUCH_NUMBER_POS   (5)
#define GT1151_TOUCH_NUMBER_MIN   (1)
#define GT1151_TOUCH_NUMBER_MAX   (5)
#define GT1151_MODULE_SWITCH1_POS (6)
#define GT1151_REFRESH_RATE_POS   (15)
#define GT1151_REFRESH_RATE_MIN   (5)
#define GT1151_REFRESH_RATE_MAX   (20)
#define GT1151_CHECK_SUM_POS      (236)

/* Max detectable simultaneous touch points */
#define GT911_I2C_MAX_POINT 5

/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/
OPERATE_RET tdd_touch_i2c_gt1151_register(char *name, TDD_TOUCH_I2C_CFG_T *cfg);


#ifdef __cplusplus
}
#endif

#endif /* __TDD_TOUCH_GT1151_H__ */
