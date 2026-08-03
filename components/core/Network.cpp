#include "core/Network.h"

#include <cstring>
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
// esp_wifi.h is only available when a WiFi driver is present (native or remote).
// On ESP32-P4 without esp_wifi_remote, CONFIG_ESP_WIFI_ENABLED is not set.
#ifdef CONFIG_ESP_WIFI_ENABLED
#  include "esp_wifi.h"
#endif

namespace core {
namespace {
constexpr const char* TAG = "Network";

#ifdef CONFIG_ESP_WIFI_ENABLED
void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    auto& self = Network::instance();
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        self.set_status(NetworkStatus::CONNECTING);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        self.set_status(NetworkStatus::DISCONNECTED);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(data);
        char ip[16]{};
        esp_ip4addr_ntoa(&event->ip_info.ip, ip, sizeof(ip));
        Network::instance().ip_str_ = ip;
        self.set_status(NetworkStatus::CONNECTED);
    }
}
#endif

esp_err_t http_event_handler(esp_http_client_event_t* evt) {
    auto* response = static_cast<std::string*>(evt->user_data);
    if (evt->event_id == HTTP_EVENT_ON_DATA && response != nullptr && evt->data_len > 0) {
        response->append(static_cast<const char*>(evt->data), evt->data_len);
    }
    return ESP_OK;
}
} // namespace

Network& Network::instance() {
    static Network inst;
    return inst;
}

void Network::set_status(NetworkStatus status) {
    status_ = status;
    if (status_cb_) {
        status_cb_(status_);
    }
}

void Network::init() {
    if (initialized_) return;
    // esp_netif and the event loop are needed by mdns, http_client, etc. — always init them.
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) { ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err)); return; }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err)); return;
    }
#ifdef CONFIG_ESP_WIFI_ENABLED
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s - WiFi unavailable", esp_err_to_name(err));
        initialized_ = true;  // netif/event-loop are up; just no WiFi driver
        return;
    }
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr);
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        initialized_ = true;
        return;
    }
    ESP_LOGI(TAG, "WiFi init OK");
#else
    ESP_LOGW(TAG, "WiFi driver not compiled in — network stack up, no WiFi");
#endif
    initialized_ = true;
}

void Network::connect(const char* ssid, const char* password) {
#ifdef CONFIG_ESP_WIFI_ENABLED
    if (!initialized_) {
        init();
        if (!initialized_) return;
    }
    wifi_config_t cfg{};
    std::strncpy(reinterpret_cast<char*>(cfg.sta.ssid), ssid, sizeof(cfg.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(cfg.sta.password), password, sizeof(cfg.sta.password) - 1);
    cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    cfg.sta.pmf_cfg.capable = true;
    cfg.sta.pmf_cfg.required = false;
    if (esp_wifi_set_config(WIFI_IF_STA, &cfg) != ESP_OK) return;
    set_status(NetworkStatus::CONNECTING);
    if (!wifi_started_) {
        wifi_started_ = true;
        if (esp_wifi_start() != ESP_OK) { wifi_started_ = false; return; }
    } else {
        esp_wifi_connect();
    }
#else
    (void)ssid; (void)password;
    ESP_LOGW(TAG, "connect() called but WiFi not available");
#endif
}

void Network::disconnect() {
#ifdef CONFIG_ESP_WIFI_ENABLED
    esp_wifi_disconnect();
    if (wifi_started_) {
        esp_wifi_stop();
        wifi_started_ = false;
    }
#endif
    set_status(NetworkStatus::DISCONNECTED);
}

int8_t Network::rssi() const {
#ifdef CONFIG_ESP_WIFI_ENABLED
    wifi_ap_record_t ap{};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        return ap.rssi;
    }
#endif
    return -127;
}

bool Network::http_get(const std::string& url, std::string& response,
                       const std::vector<std::pair<std::string, std::string>>& headers) {
    response.clear();
    esp_http_client_config_t config{};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.event_handler = http_event_handler;
    config.user_data = &response;
    config.timeout_ms = 10000;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) return false;
    for (const auto& [key, value] : headers) {
        esp_http_client_set_header(client, key.c_str(), value.c_str());
    }
    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    return err == ESP_OK && status >= 200 && status < 300;
}

} // namespace core
