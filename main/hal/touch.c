// Initialize XPT2046 (esp_lcd_touch_xpt2046) and register as an LVGL pointer device.
//
// NOTE: Adjust pin numbers, SPI host and calibration (x_min/x_max/y_min/y_max) to match your hardware.
// This example assumes ESP-IDF esp_lcd_touch_xpt2046 driver and LVGL are available.

#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "driver/spi_common.h" // SPI_HOST_AUTO / HSPI_HOST etc if needed
#include "esp_lcd_touch_xpt2046.h" // XPT2046 driver

static const char *TAG = "touch";

/* Hardware configuration - change to your wiring */
#define TOUCH_SPI_HOST  SPI3_HOST
#define TOUCH_PIN_CS    32
#define TOUCH_PIN_INT   22
#define TOUCH_PIN_MOSI  21
#define TOUCH_PIN_MISO  23
#define TOUCH_PIN_CLK   33

/* Calibration / display size - change to your display resolution */
#ifndef DISP_HOR_RES
#define DISP_HOR_RES    240
#endif
#ifndef DISP_VER_RES
#define DISP_VER_RES    320
#endif

static esp_lcd_touch_handle_t s_touch = NULL;
lv_indev_t *s_lv_indev = NULL;

static void touch_driver_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint8_t count = 0;
    esp_lcd_touch_point_data_t touch_data;
    ESP_ERROR_CHECK(esp_lcd_touch_read_data(s_touch));
    ESP_ERROR_CHECK(esp_lcd_touch_get_data(s_touch, &touch_data, &count, 1));

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

    /* 1. Initialize the SPI bus for touch controller */
    spi_bus_config_t buscfg = {
        .mosi_io_num = TOUCH_PIN_MOSI, // XPT2046 DIN
        .miso_io_num = TOUCH_PIN_MISO, // XPT2046 DOUT
        .sclk_io_num = TOUCH_PIN_CLK,  // XPT2046 CLK
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    ret = spi_bus_initialize(TOUCH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE("XPT2046", "Failed to init SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 2. Create SPI IO device for XPT2046 */
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = TOUCH_PIN_CS,
        .dc_gpio_num = -1,  // Not used for XPT2046
        .spi_mode = 0,
        .pclk_hz = 2 * 1000 * 1000,   // 2 MHz
        .trans_queue_depth = 4,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };

    ret = esp_lcd_new_panel_io_spi(TOUCH_SPI_HOST, &io_config, &tp_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE("XPT2046", "Failed to init panel IO: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 3. Create XPT2046 touch driver */
    esp_lcd_touch_config_t touch_cfg = {
        .x_max = DISP_HOR_RES,
        .y_max = DISP_VER_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = TOUCH_PIN_INT,
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
        ESP_LOGE("XPT2046", "Failed to init touch driver: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 4. Register in LVGL as input device */
    s_lv_indev = lv_indev_create();
    lv_indev_set_type(s_lv_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_lv_indev, touch_driver_read);

    return ESP_OK;
}
