#include "esp_log.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "bsp/esp_bsp_generic.h"

static const char *TAG = "main";

static void lvgl_tick(void *arg)
{
    lv_tick_inc(1);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting BSP LVGL example");

    // Initialize display + LVGL + touch
    bsp_display_start();

    // Turn on backlight
    bsp_display_backlight_on();

    // Create periodic timer for LVGL ticks
    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer;
    esp_timer_create(&tick_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, 1000); // 1ms tick

    // Create LVGL label
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello LVGL + BSP!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    ESP_LOGI(TAG, "LVGL started successfully");

    // Main LVGL loop
    while (true) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
