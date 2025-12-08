// Copyright 2025 XtraCube
// Cube OS v1 alpha

#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "hal/display.h"
#include "hal/touch.h"

#include "ui/ui_manager.hpp"
#include "ui/screens/lock_screen.hpp"

#define DISP_WIDTH 240
#define DISP_HEIGHT 320

static const char *TAG = "main";

extern esp_lcd_panel_handle_t panel_handle;
extern lv_indev_t *s_lv_indev;

extern "C" void app_main(void)
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

    // Set up the UI manager and lock screen
    UIManager ui_manager = UIManager();
    LockScreen lock_screen = LockScreen(&ui_manager);

    ui_manager.setScreen(&lock_screen);

    ui_manager.render();        
}