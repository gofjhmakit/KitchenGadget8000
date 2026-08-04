#include "services/WiFiService.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core/Network.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mdns.h"

namespace services {
namespace {
constexpr const char* TAG = "WiFiService";

void wifi_task(void* arg) {
    auto* self = static_cast<WiFiService*>(arg);
    // Init the network stack (including esp_hosted C6 SDIO handshake) here in
    // the background so it never blocks the main boot sequence or the UI.
    core::Network::instance().init();
    while (self->running_) {
        if (!self->ssid_.empty() && !core::Network::instance().is_connected() && core::Network::instance().status() != core::NetworkStatus::CONNECTING) {
            core::Network::instance().connect(self->ssid_.c_str(), self->password_.c_str());
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    self->task_handle_ = nullptr;
    vTaskDelete(nullptr);
}
}

WiFiService& WiFiService::instance() {
    static WiFiService inst;
    return inst;
}

void WiFiService::ensure_task() {
    if (task_handle_ == nullptr) {
        xTaskCreate(wifi_task, "wifi_service", 4096, this, 4, &task_handle_);
    }
}

void WiFiService::start(const char* ssid, const char* password) {
    ssid_ = ssid ? ssid : "";
    password_ = password ? password : "";
    // Network::init() (including C6 SDIO handshake) is done inside wifi_task
    // so this call returns immediately without blocking the caller.
    running_ = true;
    ensure_task();
    if (!mdns_started_) {
        if (mdns_init() == ESP_OK) {
            mdns_hostname_set("kitchengadget8000");
            mdns_instance_name_set("KitchenGadget8000");
            mdns_started_ = true;
        } else {
            ESP_LOGW(TAG, "mDNS init failed");
        }
    }
    ESP_LOGI(TAG, "WiFi service started");
}

void WiFiService::stop() {
    running_ = false;
    if (task_handle_ != nullptr) {
        xTaskAbortDelay(task_handle_);
        for (int i = 0; i < 50 && task_handle_ != nullptr; ++i) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    if (mdns_started_) {
        mdns_free();
        mdns_started_ = false;
    }
    core::Network::instance().disconnect();
}

bool WiFiService::start_wps() {
    ESP_LOGW(TAG, "WPS placeholder invoked");
    return false;
}

} // namespace services
