#include "core/Network.h"

#include <cstring>
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

namespace core {
namespace {
constexpr const char* TAG = "Network";

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
    if (initialized_) {
        return;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    initialized_ = true;
}

void Network::connect(const char* ssid, const char* password) {
    if (!initialized_) {
        init();
    }
    wifi_config_t cfg{};
    std::strncpy(reinterpret_cast<char*>(cfg.sta.ssid), ssid, sizeof(cfg.sta.ssid));
    std::strncpy(reinterpret_cast<char*>(cfg.sta.password), password, sizeof(cfg.sta.password));
    cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    cfg.sta.pmf_cfg.capable = true;
    cfg.sta.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
    set_status(NetworkStatus::CONNECTING);
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
}

void Network::disconnect() {
    esp_wifi_disconnect();
    esp_wifi_stop();
    set_status(NetworkStatus::DISCONNECTED);
}

int8_t Network::rssi() const {
    wifi_ap_record_t ap{};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        return ap.rssi;
    }
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
    if (client == nullptr) {
        return false;
    }
    for (const auto& [key, value] : headers) {
        esp_http_client_set_header(client, key.c_str(), value.c_str());
    }
    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    return err == ESP_OK && status >= 200 && status < 300;
}

} // namespace core
