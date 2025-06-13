/**
 * @file tuya_t5ai_board.c
 * @author Tuya Inc.
 * @brief Implementation of common board-level hardware registration APIs for audio, button, and LED peripherals.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "tuya_cloud_types.h"

#include "tal_api.h"

#include "tdd_audio.h"
#include "tdd_led_gpio.h"
#include "tdd_button_gpio.h"

#if defined (TUYA_T5AI_BOARD_EX_MODULE_35565LCD) && (TUYA_T5AI_BOARD_EX_MODULE_35565LCD ==1)
#include "tdd_disp_ili9488.h"
#include "tdd_touch_gt1151.h"
#elif defined (TUYA_T5AI_BOARD_EX_MODULE_EYES) && (TUYA_T5AI_BOARD_EX_MODULE_EYES ==1)
#include "tdd_disp_st7735s.h"
#endif
/***********************************************************
************************macro define************************
***********************************************************/
#define BOARD_SPEAKER_EN_PIN         TUYA_GPIO_NUM_28

#define BOARD_BUTTON_PIN             TUYA_GPIO_NUM_12
#define BOARD_BUTTON_ACTIVE_LV       TUYA_GPIO_LEVEL_LOW

#define BOARD_LED_PIN                TUYA_GPIO_NUM_1
#define BOARD_LED_ACTIVE_LV          TUYA_GPIO_LEVEL_HIGH

#if defined (TUYA_T5AI_BOARD_EX_MODULE_35565LCD) && (TUYA_T5AI_BOARD_EX_MODULE_35565LCD ==1)
#define BOARD_LCD_SW_SPI_CLK_PIN     TUYA_GPIO_NUM_49
#define BOARD_LCD_SW_SPI_CSX_PIN     TUYA_GPIO_NUM_48
#define BOARD_LCD_SW_SPI_SDA_PIN     TUYA_GPIO_NUM_50
#define BOARD_LCD_SW_SPI_DC_PIN      TUYA_GPIO_NUM_MAX
#define BOARD_LCD_SW_SPI_RST_PIN     TUYA_GPIO_NUM_MAX

#define BOARD_LCD_BL_TYPE            TUYA_DISP_BL_TP_GPIO 
#define BOARD_LCD_BL_PIN             TUYA_GPIO_NUM_9
#define BOARD_LCD_BL_ACTIVE_LV       TUYA_GPIO_LEVEL_HIGH

#define BOARD_LCD_WIDTH              320
#define BOARD_LCD_HEIGHT             480
#define BOARD_LCD_PIXELS_FMT         TUYA_PIXEL_FMT_RGB565
#define BOARD_LCD_ROTATION           TUYA_DISPLAY_ROTATION_0

#define BOARD_LCD_POWER_PIN          TUYA_GPIO_NUM_MAX

#define BOARD_TOUCH_I2C_PORT         TUYA_I2C_NUM_0
#define BOARD_TOUCH_I2C_SCL_PIN      TUYA_GPIO_NUM_13
#define BOARD_TOUCH_I2C_SDA_PIN      TUYA_GPIO_NUM_15

#elif defined (TUYA_T5AI_BOARD_EX_MODULE_EYES) && (TUYA_T5AI_BOARD_EX_MODULE_EYES ==1)
#define BOARD_LCD_BL_TYPE            TUYA_DISP_BL_TP_GPIO 
#define BOARD_LCD_BL_PIN             TUYA_GPIO_NUM_25
#define BOARD_LCD_BL_ACTIVE_LV       TUYA_GPIO_LEVEL_HIGH

#define BOARD_LCD_WIDTH              128
#define BOARD_LCD_HEIGHT             128
#define BOARD_LCD_PIXELS_FMT         TUYA_PIXEL_FMT_RGB565
#define BOARD_LCD_ROTATION           TUYA_DISPLAY_ROTATION_180

#define BOARD_LCD_QSPI_PORT           TUYA_SPI_NUM_0
#define BOARD_LCD_QSPI_CLK            48000000
#define BOARD_LCD_QSPI_CS_PIN         TUYA_GPIO_NUM_23
#define BOARD_LCD_QSPI_DC_PIN         TUYA_GPIO_NUM_7
#define BOARD_LCD_QSPI_RST_PIN        TUYA_GPIO_NUM_6

#define BOARD_LCD_PIXELS_FMT         TUYA_PIXEL_FMT_RGB565

#define BOARD_LCD_POWER_PIN          TUYA_GPIO_NUM_MAX
#endif


/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
********************function declaration********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define**********************
***********************************************************/
OPERATE_RET __board_register_audio(void)
{
    OPERATE_RET rt = OPRT_OK;

#if defined(AUDIO_CODEC_NAME)
    TDD_AUDIO_T5AI_T cfg = {0};
    memset(&cfg, 0, sizeof(TDD_AUDIO_T5AI_T));

#if defined(ENABLE_AUDIO_AEC) && (ENABLE_AUDIO_AEC == 1)
    cfg.aec_enable = 1;
#else
    cfg.aec_enable = 0;
#endif

    cfg.ai_chn      = TKL_AI_0;
    cfg.sample_rate = TKL_AUDIO_SAMPLE_16K;
    cfg.data_bits   = TKL_AUDIO_DATABITS_16;
    cfg.channel     = TKL_AUDIO_CHANNEL_MONO;

    cfg.spk_sample_rate  = TKL_AUDIO_SAMPLE_16K;
    cfg.spk_pin          = BOARD_SPEAKER_EN_PIN;
    cfg.spk_pin_polarity = TUYA_GPIO_LEVEL_LOW;

    TUYA_CALL_ERR_RETURN(tdd_audio_register(AUDIO_CODEC_NAME, cfg));
#endif
    return rt;
}

static OPERATE_RET __board_register_button(void)
{
    OPERATE_RET rt = OPRT_OK;

#if defined(BUTTON_NAME)
    BUTTON_GPIO_CFG_T button_hw_cfg = {
        .pin   = BOARD_BUTTON_PIN,
        .level = BOARD_BUTTON_ACTIVE_LV,
        .mode  = BUTTON_TIMER_SCAN_MODE,
        .pin_type.gpio_pull = TUYA_GPIO_PULLUP,
    };

    TUYA_CALL_ERR_RETURN(tdd_gpio_button_register(BUTTON_NAME, &button_hw_cfg));
#endif

    return rt;
}

static OPERATE_RET __board_register_led(void)
{ 
    OPERATE_RET rt = OPRT_OK;


#if defined(LED_NAME) 
    TDD_LED_GPIO_CFG_T led_gpio;

    led_gpio.pin   = BOARD_LED_PIN;
    led_gpio.level = BOARD_LED_ACTIVE_LV;
    led_gpio.mode  = TUYA_GPIO_PUSH_PULL;

    TUYA_CALL_ERR_RETURN(tdd_led_gpio_register(LED_NAME, &led_gpio));
#endif

    return rt;
}

#if defined (TUYA_T5AI_BOARD_EX_MODULE_35565LCD) && (TUYA_T5AI_BOARD_EX_MODULE_35565LCD ==1)
static OPERATE_RET __board_register_display(void)
{
    OPERATE_RET rt = OPRT_OK;

#if defined(DISPLAY_NAME)
    DISP_RGB_DEVICE_CFG_T display_cfg;

    memset(&display_cfg, 0, sizeof(DISP_RGB_DEVICE_CFG_T));

    display_cfg.sw_spi_cfg.spi_clk = BOARD_LCD_SW_SPI_CLK_PIN;
    display_cfg.sw_spi_cfg.spi_sda = BOARD_LCD_SW_SPI_SDA_PIN;
    display_cfg.sw_spi_cfg.spi_csx = BOARD_LCD_SW_SPI_CSX_PIN;
    display_cfg.sw_spi_cfg.spi_dc  = BOARD_LCD_SW_SPI_DC_PIN;
    display_cfg.sw_spi_cfg.spi_rst = BOARD_LCD_SW_SPI_RST_PIN;

    display_cfg.bl.type              = BOARD_LCD_BL_TYPE;
    display_cfg.bl.gpio.pin          = BOARD_LCD_BL_PIN;
    display_cfg.bl.gpio.active_level = BOARD_LCD_BL_ACTIVE_LV;

    display_cfg.width     = BOARD_LCD_WIDTH;
    display_cfg.height    = BOARD_LCD_HEIGHT;
    display_cfg.pixel_fmt = BOARD_LCD_PIXELS_FMT;
    display_cfg.rotation  = BOARD_LCD_ROTATION;

    display_cfg.power.pin = BOARD_LCD_POWER_PIN;

    TUYA_CALL_ERR_RETURN(tdd_disp_rgb_ili9488_register(DISPLAY_NAME, &display_cfg));

    TDD_TOUCH_I2C_CFG_T touch_cfg = {
        .port    = BOARD_TOUCH_I2C_PORT,
        .scl_pin = BOARD_TOUCH_I2C_SCL_PIN,
        .sda_pin = BOARD_TOUCH_I2C_SDA_PIN,
    };

    TUYA_CALL_ERR_RETURN(tdd_touch_i2c_gt1151_register(DISPLAY_NAME, &touch_cfg));
#endif

    return rt;
}

#elif defined (TUYA_T5AI_BOARD_EX_MODULE_EYES) && (TUYA_T5AI_BOARD_EX_MODULE_EYES ==1)
static OPERATE_RET __board_register_display(void)
{
    OPERATE_RET rt = OPRT_OK;

#if defined(DISPLAY_NAME)
    DISP_QSPI_DEVICE_CFG_T display_cfg;

    memset(&display_cfg, 0, sizeof(DISP_QSPI_DEVICE_CFG_T));

    display_cfg.bl.type              = BOARD_LCD_BL_TYPE;
    display_cfg.bl.gpio.pin          = BOARD_LCD_BL_PIN;
    display_cfg.bl.gpio.active_level = BOARD_LCD_BL_ACTIVE_LV;

    display_cfg.width     = BOARD_LCD_WIDTH;
    display_cfg.height    = BOARD_LCD_HEIGHT;
    display_cfg.pixel_fmt = BOARD_LCD_PIXELS_FMT;
    display_cfg.rotation  = BOARD_LCD_ROTATION;

    display_cfg.port      = BOARD_LCD_QSPI_PORT;
    display_cfg.spi_clk   = BOARD_LCD_QSPI_CLK;
    display_cfg.cs_pin    = BOARD_LCD_QSPI_CS_PIN;
    display_cfg.dc_pin    = BOARD_LCD_QSPI_DC_PIN;
    display_cfg.rst_pin   = BOARD_LCD_QSPI_RST_PIN;

    display_cfg.power.pin          = BOARD_LCD_POWER_PIN;

    TUYA_CALL_ERR_RETURN(tdd_disp_qspi_st7735s_register(DISPLAY_NAME, &display_cfg));
#endif

    return rt;
}
#else 

static OPERATE_RET __board_register_display(void)
{
    return OPRT_OK;
}

#endif

/**
 * @brief Registers all the hardware peripherals (audio, button, LED) on the board.
 * 
 * @return Returns OPERATE_RET_OK on success, or an appropriate error code on failure.
 */
OPERATE_RET board_register_hardware(void)
{
    OPERATE_RET rt = OPRT_OK;

    TUYA_CALL_ERR_LOG(__board_register_audio());

    TUYA_CALL_ERR_LOG(__board_register_button());

    TUYA_CALL_ERR_LOG(__board_register_led());

    TUYA_CALL_ERR_LOG(__board_register_display());

    return rt;
}