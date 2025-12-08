// Copyright 2025 XtraCube
// Cube OS v1 alpha

#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "hal/display.h"
#include "hal/touch.h"

#define DISP_WIDTH 240
#define DISP_HEIGHT 320

static const char *TAG = "main";

extern esp_lcd_panel_handle_t panel_handle;
extern lv_indev_t *s_lv_indev;

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "Starting Cube OS v1 alpha...");

    // Initialize the ILI9341 display with LVGL
    ESP_LOGI(TAG, "Initializing display...");
    esp_err_t ret = init_ili9341_with_lvgl();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Display initialized successfully");

    // Initialize the XPT2046 touch input
    ESP_LOGI(TAG, "Initializing touch input...");
    esp_err_t ret_touch = touch_init_xpt2046();
    if (ret_touch != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize touch input: %s", esp_err_to_name(ret_touch));
        return;
    }
    ESP_LOGI(TAG, "Touch input initialized successfully");

    // delay to ensure LVGL is ready
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "Cube OS v1 alpha started successfully");

    // Draw UI

    // blue gradient background
    lv_obj_t * bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, DISP_WIDTH, DISP_HEIGHT);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x0000FF), 0);
    lv_obj_set_style_bg_grad_color(bg, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_bg_grad_dir(bg, LV_GRAD_DIR_VER, 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bg, 0, 0);

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello, Cube OS v1 alpha!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me");
    lv_obj_center(btn_label);

    // Cursor object

    lv_obj_t * cursor = lv_img_create(lv_scr_act());
    lv_image_set_src(cursor, LV_SYMBOL_PLUS);
    lv_obj_set_size(cursor, 20, 20);
    lv_obj_set_pos(cursor, DISP_WIDTH / 2, DISP_HEIGHT / 2);
    lv_indev_set_cursor(s_lv_indev, cursor);
}