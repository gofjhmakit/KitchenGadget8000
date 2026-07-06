#pragma once
#include <string>

namespace services {

class WiFiService {
public:
    static WiFiService& instance();
    void start(const char* ssid, const char* password);
    void stop();
    bool start_wps();

public:
    WiFiService() = default;
    void ensure_task();
    std::string ssid_;
    std::string password_;
    bool running_{false};
};

} // namespace services
