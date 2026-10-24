#include "display.h"

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"
#include "esp_lvgl_port.h"
#include "driver/gpio.h"

static const char *TAG = "display";

esp_lcd_panel_handle_t panel_handle;
static esp_lcd_panel_io_handle_t io_handle;
static lv_display_t *display_handle;

esp_err_t init_ili9341_with_lvgl(void)
{
    esp_err_t ret;

    const int PIN_MOSI = 14;
    const int PIN_CLK = 27;
    const int PIN_CS = 13;
    const int PIN_DC = 12;
    const int PIN_RST = 2;
    const int PIN_BCKL = 26;
    const int PIN_MISO = -1;

    const int DISP_WIDTH = 240;
    const int DISP_HEIGHT = 320;

    const spi_host_device_t SPI_HOST = SPI2_HOST;

    // 1) init SPI bus
    spi_bus_config_t buscfg = ILI9341_PANEL_BUS_SPI_CONFIG(
        PIN_CLK,
        PIN_MOSI,
        32000
    );

    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 2) create panel io handle for SPI
    esp_lcd_panel_io_spi_config_t io_config = ILI9341_PANEL_IO_SPI_CONFIG(
        PIN_CS,
        PIN_DC,
        NULL,
        NULL);
    io_config.pclk_hz = 40 * 1000 * 1000;
        
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI_HOST, &io_config, &io_handle));

    // 3) create the ILI9341 panel driver
    esp_lcd_panel_dev_config_t ili_cfg = {
        .reset_gpio_num = PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &ili_cfg, &panel_handle));

    // 4) initialize the panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 5) simple backlight enable (GPIO)
    gpio_reset_pin(PIN_BCKL);
    gpio_set_direction(PIN_BCKL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BCKL, 1);

    // 6) Initialize LVGL port
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    // 7) Add display to LVGL
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = DISP_HEIGHT * 40,
        .double_buffer = false,
        .hres = DISP_WIDTH,
        .vres = DISP_HEIGHT,
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
    return ESP_OK;
}