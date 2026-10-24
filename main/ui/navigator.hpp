#pragma once

#include <map>
#include <vector>

#include "lvgl.h"
#include "screens.hpp"

// Called when the knob is held for 2 s. Defaults to back() on every screen.
typedef void (*HoldHandler)();

class Navigator {
public:
    static Navigator& instance();

    // Hook up the encoder (call once, with the LVGL lock held)
    void init(lv_indev_t *encoder);

    // Go to a screen, remembering the current one for back()
    void navigate(ScreenID id);
    // Go to a screen and forget the history (e.g. entering a session)
    void reset(ScreenID id);
    // Return to the previous screen, if there is one
    void back();

    // Override what a 2 s hold does on the current screen. Call from a screen builder;
    // it resets to back() on every navigation.
    void setHoldHandler(HoldHandler handler) { hold_handler = handler; }

    ScreenID getCurrent() const { return current; }
    size_t historyDepth() const { return history.size(); }
    lv_group_t *currentGroup() const { return group; }

private:
    explicit Navigator(std::map<ScreenID, ScreenBuilder> registry)
        : registry(registry) {}

    void request(ScreenID id);
    void show(ScreenID id);
    static void showPending(void *);
    static void onHold();

    std::map<ScreenID, ScreenBuilder> registry;
    std::vector<ScreenID> history;
    ScreenID current = ScreenID::BOOT;
    bool has_current = false;

    ScreenID pending = ScreenID::BOOT;
    bool has_pending = false;

    lv_indev_t *encoder = nullptr;
    lv_group_t *group = nullptr;
    HoldHandler hold_handler = nullptr;
};
