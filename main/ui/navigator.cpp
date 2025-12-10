#include "navigator.hpp"
#include "ui_root.hpp"

void Navigator::navigate(ScreenID id) {
    current = id;

    auto it = registry.find(id);
    if (it == registry.end()) return;

    auto builder = it->second;

    lv_obj_t* content_area = ui_get_content_area();
    lv_obj_clean(content_area);
    builder(content_area);
}