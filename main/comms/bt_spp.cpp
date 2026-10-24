#include "bt_spp.hpp"

#include <atomic>
#include <stdio.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_check.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "esp_spp_api.h"

#include "link.hpp"

static const char *TAG = "bt";

static const char *const SPP_SERVER_NAME = "Pomo";

// The open SPP connection, 0 when no laptop is connected
static std::atomic<uint32_t> s_handle{0};
static std::atomic<bool> s_new_connection{false};
static LineAssembler s_lines;
static char s_name[16];

static void spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
        case ESP_SPP_INIT_EVT:
            if (param->init.status == ESP_SPP_SUCCESS)
                esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
            else
                ESP_LOGE(TAG, "SPP init failed: %d", param->init.status);
            break;

        case ESP_SPP_START_EVT:
            if (param->start.status == ESP_SPP_SUCCESS) {
                esp_bt_gap_set_device_name(s_name);
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
                ESP_LOGI(TAG, "Discoverable as \"%s\"", s_name);
            } else {
                ESP_LOGE(TAG, "SPP server failed to start: %d", param->start.status);
            }
            break;

        case ESP_SPP_SRV_OPEN_EVT:
            s_handle = param->srv_open.handle;
            ESP_LOGI(TAG, "Laptop connected");
            // Not sent from here: writing to SPP from inside its own callback can block
            s_new_connection = true;
            break;

        case ESP_SPP_CLOSE_EVT:
            if (param->close.handle == s_handle) s_handle = 0;
            ESP_LOGI(TAG, "Laptop disconnected");
            break;

        case ESP_SPP_DATA_IND_EVT:
            // Runs in the Bluetooth task: parsing a short line is quick, and the handler
            // only posts to queues
            s_lines.feed((const char *)param->data_ind.data, param->data_ind.len);
            break;

        case ESP_SPP_CONG_EVT:
            if (param->cong.cong) ESP_LOGW(TAG, "Link congested");
            break;

        default:
            break;
    }
}

static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
                ESP_LOGI(TAG, "Paired with %s", (const char *)param->auth_cmpl.device_name);
            else
                ESP_LOGW(TAG, "Pairing failed: %d", param->auth_cmpl.stat);
            break;

        case ESP_BT_GAP_PIN_REQ_EVT: {
            // Old (legacy) pairing asks for a PIN: answer 1234
            esp_bt_pin_code_t pin = {'1', '2', '3', '4'};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin);
            break;
        }

        case ESP_BT_GAP_CFM_REQ_EVT:
            // Secure Simple Pairing "does this number match?": the device has no keypad, accept
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;

        default:
            break;
    }
}

esp_err_t bt_spp_init()
{
    // Classic only: hand BLE's controller memory back to the heap
    ESP_RETURN_ON_ERROR(esp_bt_controller_mem_release(ESP_BT_MODE_BLE), TAG, "release BLE memory");

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_bt_controller_init(&bt_cfg), TAG, "controller init");
    ESP_RETURN_ON_ERROR(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT), TAG, "controller enable");

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_bluedroid_init_with_cfg(&bluedroid_cfg), TAG, "bluedroid init");
    ESP_RETURN_ON_ERROR(esp_bluedroid_enable(), TAG, "bluedroid enable");

    // "Pomo-" plus the last two bytes of the Bluetooth address, so two devices differ
    const uint8_t *addr = esp_bt_dev_get_address();
    snprintf(s_name, sizeof(s_name), "Pomo-%02X%02X", addr[4], addr[5]);

    ESP_RETURN_ON_ERROR(esp_bt_gap_register_callback(gap_cb), TAG, "gap callback");
    ESP_RETURN_ON_ERROR(esp_spp_register_callback(spp_cb), TAG, "spp callback");

    esp_spp_cfg_t spp_cfg = {};
    spp_cfg.mode = ESP_SPP_MODE_CB;
    spp_cfg.enable_l2cap_ertm = true;
    spp_cfg.tx_buffer_size = 0; // VFS mode only
    ESP_RETURN_ON_ERROR(esp_spp_enhanced_init(&spp_cfg), TAG, "spp init");

    // No display-and-confirm or keypad: pair with "Just Works"
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));
    esp_bt_pin_code_t pin = {'1', '2', '3', '4'};
    esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 4, pin);

    ESP_LOGI(TAG, "Bluetooth SPP starting as \"%s\"", s_name);
    return ESP_OK;
}

bool bt_spp_send(const char *data, size_t len)
{
    const uint32_t handle = s_handle;
    if (!handle) return false;
    // esp_spp_write copies the data before returning
    return esp_spp_write(handle, (int)len, (uint8_t *)data) == ESP_OK;
}

bool bt_spp_connected()
{
    return s_handle != 0;
}

bool bt_spp_take_new_connection()
{
    return s_new_connection.exchange(false);
}
