#include "display.h"

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"
#include "esp_lvgl_port.h"
#include "driver/gpio.h"

static esp_lcd_panel_handle_t panel_handle;
static esp_lcd_panel_io_handle_t io_handle;
static lv_display_t *display_handle;

esp_err_t init_ili9341_with_lvgl(void)
{
    esp_err_t ret;
    // Pin configuration (adjust to your wiring)
    const int PIN_MOSI = 14;
    const int PIN_CLK  = 27;
    const int PIN_CS   = 13;
    const int PIN_DC   = 12;
    const int PIN_RST  = 2;
    const int PIN_BCKL = 26;
    const int PIN_MISO = -1; // not used

    const int DISP_WIDTH  = 240;
    const int DISP_HEIGHT = 320;

    // 1) init SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISP_WIDTH * DISP_HEIGHT * 2 + 8,
    };
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;

    // 2) create panel io handle for SPI
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ret = esp_lcd_new_panel_io_spi(HSPI_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) return ret;

    // 3) create the ILI9341 panel driver
    esp_lcd_panel_dev_config_t ili_cfg = {
        .reset_gpio_num = PIN_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB, // change to BGR if your panel requires it
        .bits_per_pixel = 16,
    };
    ret = esp_lcd_new_panel_ili9341(io_handle, &ili_cfg, &panel_handle);
    if (ret != ESP_OK) return ret;

    // 4) reset and initialize the panel
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, false);
    esp_lcd_panel_set_gap(panel_handle, 0, 0);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    // 5) simple backlight enable (GPIO)
    gpio_reset_pin(PIN_BCKL);
    gpio_set_direction(PIN_BCKL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BCKL, 1);

    // 6) Initialize LVGL port
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ret = lvgl_port_init(&lvgl_cfg);
    if (ret != ESP_OK) return ret;

    // 7) Add display to LVGL
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = DISP_WIDTH*40,
        .double_buffer = true,
        .hres = DISP_WIDTH,
        .vres = DISP_HEIGHT,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = false,
        }
    };
    
    display_handle = lvgl_port_add_disp(&disp_cfg);

    if (display_handle == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}