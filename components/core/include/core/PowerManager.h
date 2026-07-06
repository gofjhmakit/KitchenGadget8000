#pragma once
#include <cstdint>
#include <functional>

namespace core {

enum class PowerState {
    ACTIVE,
    DIM,
    SCREENSAVER,
    SLEEP
};

enum class ChargeState {
    UNKNOWN,
    DISCHARGING,
    CHARGING,
    FULL
};

class PowerManager {
public:
    static PowerManager& instance();
    void init();
    void update(float delta_sec);
    void reset_activity();
    float idle_seconds() const { return idle_sec_; }
    bool should_screensaver() const;
    void set_screensaver_blocked(bool blocked);
    void set_screensaver_timeout(float seconds);
    float battery_voltage() const;
    uint8_t battery_percent() const;
    ChargeState charge_state() const;
    void set_backlight(uint8_t brightness);
    uint8_t backlight() const { return backlight_; }
    PowerState state() const { return state_; }
    using StateCallback = std::function<void(PowerState)>;
    void set_state_callback(StateCallback cb) { state_cb_ = cb; }

private:
    PowerManager() = default;
    void set_state(PowerState state);
    float idle_sec_{0.0f};
    float screensaver_timeout_{120.0f};
    bool screensaver_blocked_{false};
    PowerState state_{PowerState::ACTIVE};
    ChargeState charge_state_{ChargeState::UNKNOWN};
    uint8_t backlight_{255};
    uint8_t battery_percent_{100};
    float battery_voltage_{4.2f};
    float battery_sample_accumulator_{0.0f};
    StateCallback state_cb_;
};

} // namespace core
