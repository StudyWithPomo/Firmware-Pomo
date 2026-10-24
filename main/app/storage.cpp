#include "storage.hpp"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "storage";
static const char *const NS = "pomo";

// What flash holds right now, so save() only writes what changed
static SavedState s_saved;
static bool s_have_saved = false;

void storage_defaults(SavedState *out)
{
    memset(out, 0, sizeof(*out));
    out->xp = 0;
    out->pet_body = 0;   // bot
    out->pet_face = 0;   // plain
    out->pet_finish = 0; // brass
    strcpy(out->pet_name, "Mochi");
    out->user_name[0] = '\0';
    out->study_min = 25;
    out->break_min = 5;
    out->always_show_timer = false;
}

static void get_i32(nvs_handle_t h, const char *key, int *value)
{
    int32_t v;
    if (nvs_get_i32(h, key, &v) == ESP_OK) *value = (int)v;
}

static void get_str(nvs_handle_t h, const char *key, char *buf, size_t size)
{
    size_t len = size;
    if (nvs_get_str(h, key, buf, &len) != ESP_OK) return;
    buf[size - 1] = '\0';
}

void storage_load(SavedState *out)
{
    storage_defaults(out);
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) == ESP_OK) {
        int always = 0;
        get_i32(h, "xp", &out->xp);
        get_i32(h, "body", &out->pet_body);
        get_i32(h, "face", &out->pet_face);
        get_i32(h, "finish", &out->pet_finish);
        get_str(h, "pet", out->pet_name, sizeof(out->pet_name));
        get_str(h, "user", out->user_name, sizeof(out->user_name));
        get_i32(h, "study", &out->study_min);
        get_i32(h, "break", &out->break_min);
        get_i32(h, "always", &always);
        out->always_show_timer = always != 0;
        nvs_close(h);
        ESP_LOGI(TAG, "Loaded: %d XP, pet \"%s\" (%d/%d/%d), user \"%s\", %d / %d", out->xp, out->pet_name,
                 out->pet_body, out->pet_face, out->pet_finish, out->user_name, out->study_min, out->break_min);
    } else {
        ESP_LOGI(TAG, "Nothing saved yet, using defaults");
    }
    s_saved = *out;
    s_have_saved = true;
}

bool storage_save(const SavedState &st)
{
    const SavedState &old = s_saved;
    const bool all = !s_have_saved;
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open storage: %s", esp_err_to_name(err));
        return false;
    }

    int changed = 0;
    auto put_i32 = [&](const char *key, int now, int before) {
        if (all || now != before) {
            if (err == ESP_OK) err = nvs_set_i32(h, key, now);
            changed++;
        }
    };
    auto put_str = [&](const char *key, const char *now, const char *before) {
        if (all || strcmp(now, before) != 0) {
            if (err == ESP_OK) err = nvs_set_str(h, key, now);
            changed++;
        }
    };

    put_i32("xp", st.xp, old.xp);
    put_i32("body", st.pet_body, old.pet_body);
    put_i32("face", st.pet_face, old.pet_face);
    put_i32("finish", st.pet_finish, old.pet_finish);
    put_str("pet", st.pet_name, old.pet_name);
    put_str("user", st.user_name, old.user_name);
    put_i32("study", st.study_min, old.study_min);
    put_i32("break", st.break_min, old.break_min);
    put_i32("always", st.always_show_timer ? 1 : 0, old.always_show_timer ? 1 : 0);

    if (changed && err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Save failed: %s", esp_err_to_name(err));
        return false;
    }
    if (changed) ESP_LOGI(TAG, "Saved %d value(s)", changed);
    s_saved = st;
    s_have_saved = true;
    return true;
}
