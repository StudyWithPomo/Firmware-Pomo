/*
 * /home/sumar/Cube_Phone/Cube_OS_v1_alpha/main/main.c
 * Minimal ESP32 app (ESP-IDF)
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "display.h"
#include "touch.h"

static const char *TAG = "minimal_app";

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "Hello ESP32! SDK version: %s", esp_get_idf_version());

    // Initialize the ILI9341 display with LVGL
    esp_err_t ret = init_ili9341_with_lvgl();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Display initialized successfully");

    // Initialize touch input
    lv_indev_t *touch_indev = touch_init_xpt2046();
    if (touch_indev == NULL) {
        ESP_LOGE(TAG, "Failed to initialize touch input");
        return;
    }
    ESP_LOGI(TAG, "Touch input initialized successfully");

    while (1) {
        ESP_LOGI(TAG, "Running...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}