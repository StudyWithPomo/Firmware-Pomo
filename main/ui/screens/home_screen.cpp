#include "lvgl.h"
#include "home_screen.hpp"
#include "status_bar.hpp"
#include <ui/ui_manager.hpp>
#include "lock_screen.hpp"

void HomeScreen::render() {
    lv_obj_t *scr = lv_scr_act();

    /* ===== Background (soft linen-like texture via gradients) ===== */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);

    /* ===== Status Bar reused from system renderer ===== */
    render_status_bar(true);

    /* ===== App Grid Layout ===== */
    const int ICON_SIZE = 50;
    const int ICON_PAD = 15;
    const int LABEL_PAD_TOP = 5;

    struct AppIcon {
        const char* name;
        const char* symbol;
        const int color;
        void (*onClick)() = nullptr;
    };

    const AppIcon* apps_data = new AppIcon[4] {
        {"Photos", LV_SYMBOL_IMAGE, 0xE74C3C},
        {"Files", LV_SYMBOL_DRIVE, 0x27AE60},
        {"Clock", LV_SYMBOL_BELL, 0xF1C40F},
        {"Music", LV_SYMBOL_AUDIO, 0x9B59B6},
    };

    const AppIcon *dock_apps = new AppIcon[3] {
        {"Phone", LV_SYMBOL_CALL, 0xE67E22},
        {"Settings", LV_SYMBOL_SETTINGS, 0x3498DB},
        {"Logout", LV_SYMBOL_POWER, 0x1ABC9C, []() {
            UIManager::getInstance().setScreen(LockScreen::getInstance());
        }},
    };

    for (int i = 0; i < 4; i++) {

        int row = i / 3;
        int col = i % 3;

        int x = 20 + col * (ICON_SIZE + ICON_PAD + 10);
        int y = 40 + row * (ICON_SIZE + 35);

        /* Icon container (rounded glossy box) */
        lv_obj_t *icon = lv_button_create(scr);
        lv_obj_set_size(icon, ICON_SIZE, ICON_SIZE);
        lv_obj_set_pos(icon, x, y);
        lv_obj_set_style_radius(icon, 15, 0);
        lv_obj_set_style_border_width(icon, 0, 0);
        lv_obj_set_style_bg_color(icon, lv_color_hex(apps_data[i].color), 0);
        lv_obj_set_scroll_dir(icon, LV_DIR_NONE);

        /* Click event handler */
        if (apps_data[i].onClick != nullptr) {
            lv_obj_add_event_cb(icon, [](lv_event_t * e) {
                AppIcon *app = static_cast<AppIcon*>(lv_event_get_user_data(e));
                if (app->onClick) {
                    app->onClick();
                }
            }, LV_EVENT_CLICKED, (void*)&apps_data[i]);
        }

        /* Add an LVGL symbol as placeholder emoji/icon */
        lv_obj_t *symbol = lv_label_create(icon);
        lv_label_set_text(symbol, apps_data[i].symbol);
        lv_obj_set_style_text_font(symbol, &lv_font_montserrat_28, 0);
        lv_obj_center(symbol);

        /* Icon label */
        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_text(label, apps_data[i].name);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align_to(label, icon, LV_ALIGN_OUT_BOTTOM_MID, 0, LABEL_PAD_TOP);
    }

    /* ===== Dock at the bottom ===== */
    lv_obj_t *dock_bg = lv_obj_create(scr);
    lv_obj_set_size(dock_bg, lv_pct(100), 70);
    lv_obj_align(dock_bg, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(dock_bg, lv_color_hex(0x202020), 0);
    lv_obj_set_style_border_width(dock_bg, 0, 0);
    lv_obj_set_style_radius(dock_bg, 0, 0);
    lv_obj_set_style_bg_opa(dock_bg, LV_OPA_90, 0);
    lv_obj_set_scroll_dir(dock_bg, LV_DIR_NONE);
    for (int i = 0; i < 3; i++) {
        int x = 10 + i * (ICON_SIZE + ICON_PAD + 10);

        /* Dock icon container */
        lv_obj_t *icon = lv_button_create(dock_bg);
        lv_obj_set_size(icon, ICON_SIZE, ICON_SIZE);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, x, 0);
        lv_obj_set_pos(icon, x, 0);
        lv_obj_set_style_radius(icon, 15, 0);
        lv_obj_set_style_border_width(icon, 0, 0);
        lv_obj_set_style_bg_color(icon, lv_color_hex(dock_apps[i].color), 0);
        lv_obj_set_scroll_dir(icon, LV_DIR_NONE);

        /* Click event handler */
        if (dock_apps[i].onClick != nullptr) {
            lv_obj_add_event_cb(icon, [](lv_event_t * e) {
                AppIcon *app = static_cast<AppIcon*>(lv_event_get_user_data(e));
                if (app->onClick) {
                    app->onClick();
                }
            }, LV_EVENT_CLICKED, (void*)&dock_apps[i]);
        }

        /* Add an LVGL symbol as placeholder emoji/icon */
        lv_obj_t *symbol = lv_label_create(icon);
        lv_label_set_text(symbol, dock_apps[i].symbol);
        lv_obj_set_style_text_font(symbol, &lv_font_montserrat_28, 0);
        lv_obj_center(symbol);
    }
}
