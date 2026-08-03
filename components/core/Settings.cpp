#include "core/Settings.h"

#include <cstring>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

// Pull in WiFi credentials and other secrets from secrets.h (git-ignored).
// Copy secrets.h.example → main/secrets.h and fill in your values.
#if __has_include("secrets.h")
#  include "secrets.h"
#endif

namespace core {
namespace {
constexpr const char* TAG = "Settings";
constexpr const char* KEY = "app_settings";
}

Settings& Settings::instance() {
    static Settings inst;
    return inst;
}

bool Settings::init() {
    reset_defaults();
    return true;
}

void Settings::reset_defaults() {
    settings_ = AppSettings{};
#ifdef WIFI_SSID
    std::strncpy(settings_.wifi_ssid, WIFI_SSID, sizeof(settings_.wifi_ssid) - 1);
#endif
#ifdef WIFI_PASSWORD
    std::strncpy(settings_.wifi_password, WIFI_PASSWORD, sizeof(settings_.wifi_password) - 1);
#endif
#ifdef HC_ACCESS_TOKEN
    std::strncpy(settings_.hc_access_token, HC_ACCESS_TOKEN, sizeof(settings_.hc_access_token) - 1);
#endif
#ifdef HC_APPLIANCE_ID
    std::strncpy(settings_.hc_appliance_id, HC_APPLIANCE_ID, sizeof(settings_.hc_appliance_id) - 1);
#endif
}

bool Settings::save() {
    nvs_handle_t nvs{};
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        return false;
    }
    esp_err_t err = nvs_set_blob(nvs, KEY, &settings_, sizeof(settings_));
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);
    return err == ESP_OK;
}

bool Settings::load() {
    nvs_handle_t nvs{};
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        return false;
    }
    size_t size = sizeof(settings_);
    esp_err_t err = nvs_get_blob(nvs, KEY, &settings_, &size);
    nvs_close(nvs);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        reset_defaults();
        save();
        return true;
    }
    if (err != ESP_OK || size != sizeof(settings_)) {
        ESP_LOGW(TAG, "Settings load failed, restoring defaults");
        reset_defaults();
        save();
        return false;
    }
    // Migration: if WiFi SSID is empty or still has the placeholder, override with secrets.h
#ifdef WIFI_SSID
    const bool is_placeholder = (std::strcmp(settings_.wifi_ssid, "your_network_name") == 0 ||
                                  std::strcmp(settings_.wifi_ssid, "YOUR_WIFI_SSID") == 0);
    if (settings_.wifi_ssid[0] == '\0' || is_placeholder) {
        std::strncpy(settings_.wifi_ssid, WIFI_SSID, sizeof(settings_.wifi_ssid) - 1);
#ifdef WIFI_PASSWORD
        std::strncpy(settings_.wifi_password, WIFI_PASSWORD, sizeof(settings_.wifi_password) - 1);
#endif
        save();
        ESP_LOGI(TAG, "WiFi credentials migrated from secrets.h -> %s", settings_.wifi_ssid);
    }
#endif
    return true;
}

} // namespace core
