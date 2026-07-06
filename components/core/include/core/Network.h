#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace core {

enum class NetworkStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

class Network {
public:
    static Network& instance();
    void init();
    void connect(const char* ssid, const char* password);
    void disconnect();
    NetworkStatus status() const { return status_; }
    bool is_connected() const { return status_ == NetworkStatus::CONNECTED; }
    std::string ip_address() const { return ip_str_; }
    int8_t rssi() const;
    bool http_get(const std::string& url, std::string& response,
                  const std::vector<std::pair<std::string,std::string>>& headers = {});
    using StatusCallback = std::function<void(NetworkStatus)>;
    void set_status_callback(StatusCallback cb) { status_cb_ = cb; }

public:
    Network() = default;
    void set_status(NetworkStatus status);
    NetworkStatus status_{NetworkStatus::DISCONNECTED};
    std::string ip_str_;
    StatusCallback status_cb_;
    bool initialized_{false};
};

} // namespace core
