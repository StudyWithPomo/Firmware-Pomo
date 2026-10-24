// Initialize XPT2046 (esp_lcd_touch_xpt2046) and register as an LVGL pointer device.
//
// Calibration maps raw readings to the panel's native portrait 240x320. LVGL 9 rotates
// pointer input to match the display rotation, so this stays portrait after the display
// goes landscape. If axes come out swapped or mirrored, fix touch_cfg.flags only.

#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "driver/spi_common.h"
#include "esp_lcd_touch_xpt2046.h" // XPT2046 driver

#include "board_pins.h"

static const char *TAG = "touch";

// The display driver initializes this bus; touch adds itself as a second device
#define TOUCH_SPI_HOST  BOARD_SPI_HOST

/* Panel native (portrait) size the calibration maps to */
#define DISP_HOR_RES    240
#define DISP_VER_RES    320

static esp_lcd_touch_handle_t s_touch = NULL;
lv_indev_t *s_lv_indev = NULL;

static void touch_driver_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint8_t count = 0;
    esp_lcd_touch_point_data_t touch_data;
    // A failed SPI read is treated as "released" rather than aborting
    if (esp_lcd_touch_read_data(s_touch) != ESP_OK ||
        esp_lcd_touch_get_data(s_touch, &touch_data, &count, 1) != ESP_OK) {
        count = 0;
    }

    if (count > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_data.x;
        data->point.y = touch_data.y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static uint16_t raw_x_min = 350;
static uint16_t raw_x_max = 3700;
static uint16_t raw_y_min = 200;
static uint16_t raw_y_max = 3800;

static void touch_calibration(
    esp_lcd_touch_handle_t tp,
    uint16_t *x,
    uint16_t *y,
    uint16_t *strength,
    uint8_t *point_num,
    uint8_t max_point_num
){
    if (*point_num == 0) return;
    
    // clamp raw values
    if (*x < raw_x_min) *x = raw_x_min;
    if (*x > raw_x_max) *x = raw_x_max;
    if (*y < raw_y_min) *y = raw_y_min;
    if (*y > raw_y_max) *y = raw_y_max;

    // map raw -> LVGL coords
    *x = (*x - raw_x_min) * DISP_HOR_RES / (raw_x_max - raw_x_min);
    *y = (*y - raw_y_min) * DISP_VER_RES / (raw_y_max - raw_y_min);
}

esp_err_t touch_init_xpt2046(void)
{
    esp_err_t ret;

    /* 1. Join the display's SPI bus (call after init_ili9341_with_lvgl) as a second device */
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = PIN_TOUCH_CS,
        .dc_gpio_num = -1,  // Not used for XPT2046
        .spi_mode = 0,
        .pclk_hz = 2 * 1000 * 1000,   // 2 MHz
        .trans_queue_depth = 4,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };

    ret = esp_lcd_new_panel_io_spi(TOUCH_SPI_HOST, &io_config, &tp_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init panel IO: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 2. Create XPT2046 touch driver */
    esp_lcd_touch_config_t touch_cfg = {
        .x_max = DISP_HOR_RES,
        .y_max = DISP_VER_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = PIN_TOUCH_IRQ,
        .levels = {
            .reset = 0,
            .interrupt = 0, // IRQ active low
        },
        .flags = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .process_coordinates = touch_calibration,
    };

    ret = esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &touch_cfg, &s_touch);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init touch driver: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 3. Register in LVGL as input device */
    lvgl_port_lock(0);
    s_lv_indev = lv_indev_create();
    lv_indev_set_type(s_lv_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_lv_indev, touch_driver_read);
    lvgl_port_unlock();

    return ESP_OK;
}
