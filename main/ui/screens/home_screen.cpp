#include <ui/screens.hpp>
#include <ui/app_state.hpp>
#include <ui/ui_root.hpp>
#include <ui/wallpapers.h>
#include <ui/fonts.h>
#include <ui/assets.h>

#include "lvgl.h"
#include "esp_log.h"

struct AppIcon
{
    const char *label;
    const char *symbol;
    void (*action)(lv_event_t *e);
    lv_color_t color = lv_palette_main(LV_PALETTE_BLUE);
};

void draw_app_icon(const AppIcon *app, lv_obj_t *parent, int x, int y, int icon_size, int icon_radius) {
    lv_obj_t *app_btn = lv_btn_create(parent);
    lv_obj_align(app_btn, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_set_size(app_btn, icon_size, icon_size);
    lv_obj_set_style_bg_color(app_btn, app->color, 0);
    lv_obj_set_style_radius(app_btn, icon_radius, 0);
    lv_obj_set_style_clip_corner(app_btn, true, 0);
    lv_obj_add_flag(app_btn, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_event_cb(app_btn, app->action, LV_EVENT_CLICKED, nullptr);

    lv_obj_set_style_transform_width(app_btn, 0, LV_STATE_PRESSED);
    lv_obj_set_style_transform_height(app_btn, 0, LV_STATE_PRESSED);

    lv_obj_t *app_btn_bg = lv_img_create(app_btn);
    lv_img_set_src(app_btn_bg, &app_overlay);
    lv_obj_set_size(app_btn_bg, icon_size, icon_size);
    lv_obj_center(app_btn_bg);

    lv_obj_t *app_icon_label = lv_label_create(app_btn);
    lv_label_set_text(app_icon_label, app->symbol);
    lv_obj_set_style_text_font(app_icon_label, &ext_symbols_28, 0);
    lv_obj_center(app_icon_label);

    lv_obj_t *text_shadow2 = lv_label_create(parent);
    lv_obj_t *text_shadow3 = lv_label_create(parent);
    lv_obj_t *text_shadow4 = lv_label_create(parent);
    lv_obj_t *app_text = lv_label_create(parent);

    lv_label_set_text(text_shadow2, app->label);
    lv_label_set_text(text_shadow3, app->label);
    lv_label_set_text(text_shadow4, app->label);
    lv_label_set_text(app_text, app->label);

    static lv_style_t style_shadow;
    lv_style_init(&style_shadow);

    lv_style_set_text_align(&style_shadow, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_font(&style_shadow, &lv_font_montserrat_12);
    lv_style_set_text_color(&style_shadow, lv_color_black());
    lv_style_set_text_opa(&style_shadow, LV_OPA_40);

    lv_obj_add_style(text_shadow2, &style_shadow, 0);
    lv_obj_add_style(text_shadow3, &style_shadow, 0);
    lv_obj_add_style(text_shadow4, &style_shadow, 0);

    lv_obj_set_style_text_align(app_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(app_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(app_text, lv_color_white(), 0);

    lv_obj_align_to(app_text, app_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    lv_obj_align_to(text_shadow2, app_text, LV_ALIGN_CENTER, -1, 1);
    lv_obj_align_to(text_shadow3, app_text, LV_ALIGN_CENTER, 1, 1);
    lv_obj_align_to(text_shadow4, app_text, LV_ALIGN_CENTER, 0, -1);
}

void screen_home(lv_obj_t *parent)
{
    ui_toggle_time_display(true);

    lv_obj_t *bg = lv_img_create(parent);
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));
    lv_img_set_src(bg, &gray_arc);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);

    lv_obj_t *dock = lv_obj_create(parent);
    lv_obj_set_size(dock, lv_pct(100), 45);
    lv_obj_align(dock, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_border_width(dock, 0, 0);
    lv_obj_set_style_radius(dock, 0, 0);
    lv_obj_set_style_bg_color(dock, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(dock, LV_OPA_70, 0);
    lv_obj_set_style_bg_grad_color(dock, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_grad_opa(dock, LV_OPA_90, 0);
    lv_obj_set_style_bg_grad_dir(dock, LV_GRAD_DIR_VER, 0);

    const AppIcon dock_apps[] = {
        {"Settings", LV_SYMBOL_SETTINGS, [](lv_event_t *e) {}, lv_palette_main(LV_PALETTE_ORANGE)},
        {"Phone", LV_SYMBOL_CALL, [](lv_event_t *e) {}, lv_palette_main(LV_PALETTE_GREEN)},
        {"Power", UI_SYMBOL_LOCK, [](lv_event_t *e)
         { AppState::instance().lockPhone(); }, lv_palette_main(LV_PALETTE_RED)},
    };

    const AppIcon page_apps[] = {
        {"SMS", UI_SYMBOL_COMMENT, [](lv_event_t *e) {}, lv_palette_main(LV_PALETTE_BLUE)},
        {"Gallery", LV_SYMBOL_IMAGE, [](lv_event_t *e) {}, lv_palette_main(LV_PALETTE_PINK)},
        {"Music", LV_SYMBOL_AUDIO, [](lv_event_t *e) {}, lv_palette_main(LV_PALETTE_PURPLE)},
        {"Files", LV_SYMBOL_DIRECTORY, [](lv_event_t *e) {}, lv_palette_main(LV_PALETTE_BROWN)},
    };

    constexpr int icon_gap = 15;

    const int s_width = lv_obj_get_width(parent);
    const int icon_size = (s_width - icon_gap * 4) / 3;
    const int icon_radius = icon_size / 5;

    constexpr int dock_count = sizeof(dock_apps) / sizeof(AppIcon);
    constexpr int page_count = sizeof(page_apps) / sizeof(AppIcon);

    lv_obj_t *page_apps_container = lv_obj_create(parent);
    lv_obj_set_size(page_apps_container, lv_pct(100), lv_pct(100));
    lv_obj_align(page_apps_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(page_apps_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page_apps_container, 0, 0);
    lv_obj_set_style_pad_hor(page_apps_container, icon_gap, 0);
    lv_obj_set_style_pad_bottom(page_apps_container, 85 + icon_gap, 0); // Leave space for dock
    lv_obj_set_style_pad_top(page_apps_container, 24 + icon_gap, 0); // Leave space for status bar

    for (int i = 0; i < page_count; ++i)
    {
        const AppIcon &app = page_apps[i];

        const int x = (i % 3) * (icon_size + icon_gap);
        const int y = (i / 3) * (icon_size + 10 + icon_gap);

        draw_app_icon(&app, page_apps_container, x, y,  icon_size, icon_radius);
    }

    lv_obj_t *dock_apps_container = lv_obj_create(parent);
    lv_obj_set_size(dock_apps_container, lv_pct(100), 85);
    lv_obj_align(dock_apps_container, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_opa(dock_apps_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dock_apps_container, 0, 0);
    lv_obj_set_style_pad_hor(dock_apps_container, icon_gap, 0);
    lv_obj_set_style_pad_ver(dock_apps_container, 0, 0);

    for (int i = 0; i < dock_count; ++i)
    {
        const AppIcon &app = dock_apps[i];

        const int x = (i % dock_count) * (icon_size + icon_gap);

        draw_app_icon(&app, dock_apps_container, x, 5, icon_size, icon_radius);
    }
}