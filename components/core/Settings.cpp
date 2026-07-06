#include "core/Settings.h"

#include <cstring>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

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
        save();
        return true;
    }
    if (err != ESP_OK || size != sizeof(settings_)) {
        ESP_LOGW(TAG, "Settings load failed, restoring defaults");
        reset_defaults();
        save();
        return false;
    }
    return true;
}

} // namespace core
