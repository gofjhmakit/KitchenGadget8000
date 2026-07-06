#include "core/PowerManager.h"

#include <algorithm>
#include "esp_timer.h"
#include "hal/Display.h"

namespace core {

PowerManager& PowerManager::instance() {
    static PowerManager inst;
    return inst;
}

void PowerManager::init() {
    set_backlight(backlight_);
    charge_state_ = ChargeState::DISCHARGING;
    battery_voltage_ = 4.08f;
    battery_percent_ = 88;
}

void PowerManager::set_state(PowerState state) {
    if (state_ == state) {
        return;
    }
    state_ = state;
    if (state_cb_) {
        state_cb_(state_);
    }
}

void PowerManager::update(float delta_sec) {
    idle_sec_ += delta_sec;
    battery_sample_accumulator_ += delta_sec;
    if (battery_sample_accumulator_ >= 15.0f) {
        battery_sample_accumulator_ = 0.0f;
        battery_voltage_ = std::max(3.75f, battery_voltage_ - 0.002f);
        battery_percent_ = static_cast<uint8_t>(std::clamp((battery_voltage_ - 3.3f) / (4.2f - 3.3f) * 100.0f, 0.0f, 100.0f));
    }

    if (screensaver_blocked_) {
        if (state_ != PowerState::ACTIVE) {
            set_state(PowerState::ACTIVE);
            set_backlight(255);
        }
        return;
    }

    if (idle_sec_ >= screensaver_timeout_) {
        set_state(PowerState::SCREENSAVER);
        set_backlight(24);
    } else if (idle_sec_ >= screensaver_timeout_ * 0.5f) {
        set_state(PowerState::DIM);
        set_backlight(std::max<uint8_t>(32, backlight_ / 3));
    } else {
        set_state(PowerState::ACTIVE);
        set_backlight(backlight_);
    }
}

void PowerManager::reset_activity() {
    idle_sec_ = 0.0f;
    set_state(PowerState::ACTIVE);
    set_backlight(backlight_);
}

bool PowerManager::should_screensaver() const {
    return !screensaver_blocked_ && idle_sec_ >= screensaver_timeout_;
}

void PowerManager::set_screensaver_blocked(bool blocked) {
    screensaver_blocked_ = blocked;
}

void PowerManager::set_screensaver_timeout(float seconds) {
    screensaver_timeout_ = seconds;
}

float PowerManager::battery_voltage() const { return battery_voltage_; }
uint8_t PowerManager::battery_percent() const { return battery_percent_; }
ChargeState PowerManager::charge_state() const { return charge_state_; }

void PowerManager::set_backlight(uint8_t brightness) {
    backlight_ = brightness;
    hal::Display::instance().set_backlight(brightness);
}

} // namespace core
