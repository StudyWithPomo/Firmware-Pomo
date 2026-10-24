#include "navigator.hpp"
#include "ui_root.hpp"
#include "hal/encoder.h"

#include "esp_log.h"

static const char *TAG = "nav";

// Deep enough for Home -> Pet -> Wardrobe and back; older entries are dropped
static const size_t MAX_HISTORY = 8;

Navigator& Navigator::instance() {
    static Navigator inst({
        {ScreenID::BOOT, screen_boot},
        {ScreenID::HOME, screen_home},
        {ScreenID::STUDY, screen_study},
        {ScreenID::FOCUS, screen_focus},
        {ScreenID::INTERMISSION, screen_intermission},
        {ScreenID::READY, screen_ready},
        {ScreenID::CIRCLE, screen_circle},
        {ScreenID::PET, screen_pet},
        {ScreenID::STATUS, screen_status},
        {ScreenID::WARDROBE, screen_wardrobe},
        {ScreenID::PLAYGROUND, screen_playground},
    });
    return inst;
}

const char *screen_name(ScreenID id) {
    switch (id) {
        case ScreenID::BOOT: return "Boot";
        case ScreenID::HOME: return "Home";
        case ScreenID::STUDY: return "Study";
        case ScreenID::FOCUS: return "Focus";
        case ScreenID::INTERMISSION: return "Intermission";
        case ScreenID::READY: return "Ready";
        case ScreenID::CIRCLE: return "Study Circle";
        case ScreenID::PET: return "Pet";
        case ScreenID::STATUS: return "Status";
        case ScreenID::WARDROBE: return "Wardrobe";
        case ScreenID::PLAYGROUND: return "Playground";
        default: return "?";
    }
}

void Navigator::init(lv_indev_t *encoder_indev) {
    encoder = encoder_indev;
    encoder_set_hold_cb(onHold);
}

void Navigator::navigate(ScreenID id) {
    // Boot is a one-off greeting: Back from where it leads goes Home instead
    if (has_current && current != ScreenID::BOOT) {
        history.push_back(current);
        if (history.size() > MAX_HISTORY) history.erase(history.begin());
    }
    request(id);
}

void Navigator::reset(ScreenID id) {
    history.clear();
    request(id);
}

void Navigator::back() {
    if (history.empty()) {
        if (current != ScreenID::HOME) request(ScreenID::HOME);
        return;
    }
    ScreenID prev = history.back();
    history.pop_back();
    request(prev);
}

// Screens are swapped on the next LVGL cycle, never inside the event that asked for it,
// so a button's click handler can navigate without deleting itself mid-event.
void Navigator::request(ScreenID id) {
    pending = id;
    if (!has_pending) {
        has_pending = true;
        lv_async_call(showPending, nullptr);
    }
}

void Navigator::showPending(void *) {
    Navigator &nav = instance();
    nav.has_pending = false;
    nav.show(nav.pending);
}

void Navigator::show(ScreenID id) {
    auto it = registry.find(id);
    if (it == registry.end()) {
        ESP_LOGW(TAG, "No screen registered for %s", screen_name(id));
        return;
    }

    current = id;
    has_current = true;
    hold_handler = nullptr;

    // Tear down the old screen. Deleting its objects also removes their subject observers.
    if (encoder) lv_indev_set_group(encoder, nullptr);
    if (group) lv_group_delete(group);
    lv_obj_t *content_area = ui_get_content_area();
    lv_obj_clean(content_area);

    group = lv_group_create();
    lv_group_set_wrap(group, true);
    it->second(content_area, group);
    if (encoder) lv_indev_set_group(encoder, group);

    ESP_LOGI(TAG, "-> %s (history %u)", screen_name(id), (unsigned)history.size());
}

void Navigator::onHold() {
    Navigator &nav = instance();
    ESP_LOGI(TAG, "Hold on %s", screen_name(nav.current));
    if (nav.hold_handler) nav.hold_handler();
    else nav.back();
}
