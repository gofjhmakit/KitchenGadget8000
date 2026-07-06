#pragma once
#include <cstdint>

namespace core {

struct AppSettings {
    char wifi_ssid[64]{};
    char wifi_password[64]{};
    char hc_access_token[256]{};
    char hc_appliance_id[64]{};
    float weather_lat{62.1302f};
    float weather_lon{25.6728f};
    char weather_city[32]{"Muurame"};
    uint8_t backlight{200};
    float screensaver_timeout{120.0f};
    bool use_fahrenheit{false};
    char github_repo[128]{"gofjhmakit/KitchenGadget8000"};
    char github_branch[32]{"main"};
    uint32_t github_sync_interval_sec{3600};
};

class Settings {
public:
    static Settings& instance();
    bool init();
    bool save();
    bool load();
    void reset_defaults();
    AppSettings& get() { return settings_; }
    const AppSettings& get() const { return settings_; }

private:
    Settings() = default;
    AppSettings settings_{};
    static constexpr const char* NVS_NAMESPACE = "kg8000";
};

} // namespace core
