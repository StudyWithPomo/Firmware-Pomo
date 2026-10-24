#pragma once
// Host stand-in for ESP-IDF's esp_err.h (preview build only)
#include <stdio.h>
#include <stdlib.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
static inline const char *esp_err_to_name(esp_err_t e) { return e == ESP_OK ? "ESP_OK" : "ESP_FAIL"; }
#define ESP_ERROR_CHECK(x) do { esp_err_t err_ = (x); if (err_ != ESP_OK) { fprintf(stderr, "ESP_ERROR_CHECK failed: %s\n", #x); abort(); } } while (0)
