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

static void btn_event_cb(lv_event_t *e)
{
    ESP_LOGI("LVGL", "Button clicked!");
}

void create_ui(void)
{
    lv_obj_t *scr = lv_scr_act();

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_center(btn);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Click Me");
    lv_obj_center(label);
}

void app_main(void)
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
    
    lv_indev_t *touch_indev = touch_init_xpt2046();
    if (touch_indev == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize touch input");
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