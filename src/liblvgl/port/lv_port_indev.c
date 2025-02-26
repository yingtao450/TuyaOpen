/**
 * @file lv_port_indev_templ.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "tuya_iot_config.h"
#include "lv_port_indev.h"
#ifdef LVGL_ENABLE_TOUCH
#include "tkl_touch.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

static void encoder_init(void);
static void encoder_read(lv_indev_t *indev, lv_indev_data_t *data);
static void encoder_handler(void);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t *indev_touchpad;
lv_indev_t *indev_encoder;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    /*------------------
     * Touchpad
     * -----------------*/
#ifdef LVGL_ENABLE_TOUCH
    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);
#endif

    /*------------------
     * Encoder
     * -----------------*/
#ifdef LVGL_ENABLE_ENCODER
    /*Initialize your encoder if you have*/
    encoder_init();

    /*Register a encoder input device*/
    indev_encoder = lv_indev_create();
    lv_indev_set_type(indev_encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_encoder, encoder_read);
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/
#ifdef LVGL_ENABLE_TOUCH

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    tkl_touch_init();
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    static int32_t last_x = 0;
    static int32_t last_y = 0;
    uint8_t point_num = 0;
    touch_point_t point;

    tkl_touch_read(&point_num, &point, 1);
    /*Save the pressed coordinates and the state*/
    if (point_num > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        last_x = point.x;
        last_y = point.y;
        PR_DEBUG("touchpad_read: x=%d, y=%d", point.x, point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}
#endif

/*------------------
 * Encoder
 * -----------------*/
#ifdef LVGL_ENABLE_ENCODER
int32_t encoder_diff = 0;
lv_indev_state_t encoder_state;
/*Initialize your encoder*/
static void encoder_init(void)
{
    drv_encoder_init();
}

/*Will be called by the library to read the encoder*/
static void encoder_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    static int32_t last_diff = 0;
    int32_t diff;
    if (encoder_get_pressed()) {
        encoder_diff = 0;
        encoder_state = LV_INDEV_STATE_PRESSED;
    } else {
        diff = encoder_get_angle();

        encoder_diff = diff - last_diff;
        last_diff = diff;

        encoder_state = LV_INDEV_STATE_RELEASED;
    }

    data->enc_diff = encoder_diff;
    data->state = encoder_state;
}
#endif
