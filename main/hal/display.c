#include "display.h"

#include "board.h"

#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "display";

// Panel native size (portrait). LVGL rotates it to 320x240 landscape.
#define LCD_H_RES 240
#define LCD_V_RES 320

#define BL_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BL_LEDC_TIMER    LEDC_TIMER_0
#define BL_LEDC_CHANNEL  LEDC_CHANNEL_0
#define BL_LEDC_FREQ_HZ  5000
#define BL_LEDC_RES      LEDC_TIMER_8_BIT

esp_lcd_panel_handle_t panel_handle;
static esp_lcd_panel_io_handle_t io_handle;
static lv_display_t *display_handle;
static bool s_has_backlight;

static esp_err_t backlight_init(void)
{
    const ledc_timer_config_t timer_cfg = {
        .speed_mode = BL_LEDC_MODE,
        .duty_resolution = BL_LEDC_RES,
        .timer_num = BL_LEDC_TIMER,
        .freq_hz = BL_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "backlight timer");

    const ledc_channel_config_t channel_cfg = {
        .gpio_num = board()->lcd_bl,
        .speed_mode = BL_LEDC_MODE,
        .channel = BL_LEDC_CHANNEL,
        .timer_sel = BL_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_cfg), TAG, "backlight channel");
    s_has_backlight = true;
    return ESP_OK;
}

void display_set_brightness(uint8_t pct)
{
    // Board 2 ties the backlight to 3V3, so there's nothing to dim
    if (!s_has_backlight) return;
    if (pct > 100) pct = 100;
    const uint32_t max_duty = (1u << BL_LEDC_RES) - 1;
    ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, max_duty * pct / 100);
    ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL);
}

esp_err_t init_ili9341_with_lvgl(void)
{
    const spi_host_device_t LCD_SPI_HOST = BOARD_SPI_HOST;

    // 1) init the SPI bus the display shares with touch (touch_init_xpt2046 joins it)
    spi_bus_config_t buscfg = ILI9341_PANEL_BUS_SPI_CONFIG(
        PIN_SPI_CLK,
        PIN_SPI_MOSI,
        32000
    );
    buscfg.miso_io_num = PIN_SPI_MISO; // for touch readings

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 2) create panel io handle for SPI
    esp_lcd_panel_io_spi_config_t io_config = ILI9341_PANEL_IO_SPI_CONFIG(
        board()->lcd_cs,
        board()->lcd_dc,
        NULL,
        NULL);
    io_config.pclk_hz = 40 * 1000 * 1000;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_SPI_HOST, &io_config, &io_handle));

    // 3) create the ILI9341 panel driver
    esp_lcd_panel_dev_config_t ili_cfg = {
        .reset_gpio_num = board()->lcd_rst,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &ili_cfg, &panel_handle));

    // 4) initialize the panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 5) PWM backlight, full brightness to start (board 2 ties the backlight to
    //    3V3, so there's nothing to drive)
    if (board()->lcd_bl != PIN_NONE) {
        ESP_ERROR_CHECK(backlight_init());
        display_set_brightness(100);
    }

    // 6) Initialize LVGL port
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    // LVGL draws by recursing through the object tree, about 0.5 KB of stack per level of
    // nesting. The default 7 KB overflowed on deeply nested screens (the Workshop).
    lvgl_cfg.task_stack = 16 * 1024;
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 7) Add display to LVGL. hres/vres and .rotation describe the panel as initialized
    // (portrait); the landscape rotation is applied afterwards.
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_V_RES * 40, // 40 rows of the 320 px landscape width
        .double_buffer = false,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        }};

    display_handle = lvgl_port_add_disp(&disp_cfg);
    if (display_handle == NULL)
    {
        return ESP_FAIL;
    }

    // 8) Landscape 320x240. Hardware rotation (esp_lcd swap/mirror), no extra buffer.
    // If the image is upside down, use LV_DISPLAY_ROTATION_270.
    lvgl_port_lock(0);
    lv_display_set_rotation(display_handle, LV_DISPLAY_ROTATION_90);
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Display ready: %dx%d",
             (int)lv_display_get_horizontal_resolution(display_handle),
             (int)lv_display_get_vertical_resolution(display_handle));
    return ESP_OK;
}
