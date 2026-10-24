// Pomo firmware (Study Buddy). Based on Cube OS v1 alpha by XtraCube.

#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "system/system.hpp"

#include "hal/display.h"
#include "hal/touch.h"
#include "hal/encoder.h"

#include "ui/app_state.hpp"
#include "ui/ui_root.hpp"
#include "ui/navigator.hpp"
#include "ui/screens.hpp"
#include "ui/theme.hpp"
#include "app/session.hpp"
#include "comms/link.hpp"
#include "comms/bt_spp.hpp"

// Set to 1 to boot into the touch test screen (tap the corners to check rotation and axes)
#ifndef POMO_TOUCH_TEST
#define POMO_TOUCH_TEST 0
#endif

static const char *TAG = "main";

static void log_heap(const char *stage)
{
    ESP_LOGI(TAG, "[heap] %s: free %u, largest block %u, min ever %u", stage,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
}

extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "Starting Pomo...");
    log_heap("boot");

    // NVS: Bluetooth keeps pairing keys here (and saved progress will live here too)
    esp_err_t nvs = nvs_flash_init();
    if (nvs == ESP_ERR_NVS_NO_FREE_PAGES || nvs == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs);

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

    Session::instance().init();

    // The LVGL task is already running, so every LVGL call from here needs the lock
    lvgl_port_lock(0);
    esp_err_t ret_enc = encoder_init();
    if (ret_enc != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize encoder: %s", esp_err_to_name(ret_enc));
    }
    theme::init();
    ui_root_init();
#if POMO_TOUCH_TEST
    screen_touch_test(ui_get_content_area());
#else
    Navigator::instance().init(encoder_get_indev());
    AppState::instance().attach();
    Navigator::instance().reset(ScreenID::BOOT);
#endif
    lv_timer_create([](lv_timer_t* t){ AppState::instance().update(); }, 1000, nullptr);
    lvgl_port_unlock();

    log_heap("display + touch + LVGL");

#if !POMO_TOUCH_TEST
    link_init();
    log_heap("laptop link");

    esp_err_t ret_bt = bt_spp_init();
    if (ret_bt != ESP_OK)
    {
        // Keep running: the device works without Bluetooth, over USB only
        ESP_LOGE(TAG, "Bluetooth failed to start: %s", esp_err_to_name(ret_bt));
    }
    log_heap("bluetooth");
#endif
}