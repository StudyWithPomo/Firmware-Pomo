/*
 * /home/sumar/Cube_Phone/Cube_OS_v1_alpha/main/main.c
 * Minimal ESP32 app (ESP-IDF)
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "display.h"
#include "touch.h"

#define DISP_WIDTH 240
#define DISP_HEIGHT 320

static const char *TAG = "minimal_app";

extern esp_lcd_panel_handle_t panel_handle;
extern lv_indev_t *s_lv_indev;

static int counter = 0;
static void clicker_button_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);

    if (code == LV_EVENT_CLICKED)
    {
        counter++;
        char buf[32];
        snprintf(buf, sizeof(buf), "Clicked: %d", counter);
        lv_label_set_text(label, buf);
    }
}

void create_ui(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    // Create Windows 7 style background
    lv_obj_t *bg = lv_obj_create(scr);
    lv_obj_set_size(bg, DISP_WIDTH, DISP_HEIGHT);
    lv_obj_align(bg, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x003DA5), 0);
    lv_obj_set_style_bg_grad_color(bg, lv_color_hex(0x1E90FF), 0);
    lv_obj_set_style_bg_grad_dir(bg, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);

    // Create status bar
    lv_obj_t *status_bar = lv_obj_create(scr);
    lv_obj_set_size(status_bar, DISP_WIDTH, 20);
    lv_obj_align(status_bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_grad_color(status_bar, lv_color_hex(0xC0C0C0), 0);
    lv_obj_set_style_bg_grad_dir(status_bar, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);

    // Create a label in the status bar
    lv_obj_t *status_label = lv_label_create(status_bar);
    lv_label_set_text(status_label, "12:00 PM");
    lv_obj_align(status_label, LV_ALIGN_TOP_RIGHT, 0, -10);

    lv_obj_t *carrier_label = lv_label_create(status_bar);
    lv_label_set_text(carrier_label, "Mint");
    lv_obj_align(carrier_label, LV_ALIGN_TOP_LEFT, 0, -10);

    lv_obj_t *clicker_button = lv_btn_create(scr);
    lv_obj_set_size(clicker_button, 100, 50);
    lv_obj_align(clicker_button, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(clicker_button, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_bg_grad_color(clicker_button, lv_color_hex(0xA0A0A0), 0);
    lv_obj_set_style_bg_grad_dir(clicker_button, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(clicker_button, lv_color_hex(0x606060), 0);
    lv_obj_set_style_border_width(clicker_button, 2, 0);
    lv_obj_set_style_radius(clicker_button, 8, 0);
    lv_obj_add_event_cb(clicker_button, clicker_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(clicker_button);
    lv_label_set_text(btn_label, "Clicked: 0");
    lv_obj_center(btn_label);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0x000000), 0);
}

extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "Hello ESP32! SDK version: %s", esp_get_idf_version());

    // Initialize the ILI9341 display with LVGL
    esp_err_t ret = init_ili9341_with_lvgl();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Display initialized successfully");

    esp_err_t ret_touch = touch_init_xpt2046();
    if (ret_touch != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize touch input: %s", esp_err_to_name(ret_touch));
        return;
    }
    ESP_LOGI(TAG, "Touch input initialized successfully");

    // delay to ensure LVGL is ready
    vTaskDelay(pdMS_TO_TICKS(100));

    lvgl_port_lock(0);

    // Create the UI
    create_ui();

    lvgl_port_unlock();
}