#include "core/PowerManager.h"

#include <algorithm>
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hal/Display.h"
#include "sdkconfig.h"

namespace core {
namespace {
constexpr const char* TAG = "PowerManager";
// Clamp helper (not std::clamp to avoid float promotion warnings)
float fclamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
} // namespace

PowerManager& PowerManager::instance() {
    static PowerManager inst;
    return inst;
}

void PowerManager::init() {
    set_backlight(backlight_);

#if CONFIG_KG8000_BATTERY_ADC_CHANNEL >= 0
    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_1;
    adc_oneshot_unit_handle_t handle = nullptr;
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &handle);
    if (err == ESP_OK) {
        adc_oneshot_chan_cfg_t chan_cfg{};
        chan_cfg.atten = ADC_ATTEN_DB_12;
        chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
        err = adc_oneshot_config_channel(handle, static_cast<adc_channel_t>(CONFIG_KG8000_BATTERY_ADC_CHANNEL), &chan_cfg);
        if (err == ESP_OK) {
            adc_handle_ = handle;
            hardware_battery_ = true;
            ESP_LOGI(TAG, "Battery ADC initialised (ADC1 channel %d)", CONFIG_KG8000_BATTERY_ADC_CHANNEL);
        } else {
            ESP_LOGW(TAG, "ADC channel config failed: %s", esp_err_to_name(err));
            adc_oneshot_del_unit(handle);
        }
    } else {
        ESP_LOGW(TAG, "ADC unit init failed: %s", esp_err_to_name(err));
    }
#endif

#if CONFIG_KG8000_CHARGE_GPIO >= 0
    gpio_config_t io_conf{};
    io_conf.pin_bit_mask = 1ULL << CONFIG_KG8000_CHARGE_GPIO;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&io_conf) == ESP_OK) {
        hardware_charge_ = true;
        ESP_LOGI(TAG, "Charge-detect GPIO %d initialised", CONFIG_KG8000_CHARGE_GPIO);
    }
#endif

    // Take an initial sample so the UI has real values from the first frame.
    sample_battery();
}

void PowerManager::sample_battery() {
    if (hardware_battery_ && adc_handle_ != nullptr) {
        int raw = 0;
        auto* handle = static_cast<adc_oneshot_unit_handle_t>(adc_handle_);
        if (adc_oneshot_read(handle, static_cast<adc_channel_t>(CONFIG_KG8000_BATTERY_ADC_CHANNEL), &raw) == ESP_OK) {
            constexpr float VREF = 3.3f;
            constexpr float ADC_MAX = 4095.0f;
            const float divider = CONFIG_KG8000_BATTERY_DIVIDER_RATIO_X10 / 10.0f;
            battery_voltage_ = (raw / ADC_MAX) * VREF * divider;
            battery_percent_ = static_cast<uint8_t>(fclamp((battery_voltage_ - 3.3f) / (4.2f - 3.3f) * 100.0f, 0.0f, 100.0f));
        }
    } else if (!hardware_battery_) {
        // Simulate slow discharge so the display is not completely static in development.
        battery_voltage_ = std::max(3.55f, battery_voltage_ - 0.002f);
        battery_percent_ = static_cast<uint8_t>(fclamp((battery_voltage_ - 3.3f) / (4.2f - 3.3f) * 100.0f, 0.0f, 100.0f));
    }

    if (hardware_charge_) {
#if CONFIG_KG8000_CHARGE_GPIO >= 0
        const int level = gpio_get_level(static_cast<gpio_num_t>(CONFIG_KG8000_CHARGE_GPIO));
        // CHRG pin: LOW = charging, HIGH = idle/full.
        // We infer FULL when pin is HIGH and voltage is near maximum.
        if (level == 0) {
            charge_state_ = ChargeState::CHARGING;
        } else if (battery_percent_ >= 98) {
            charge_state_ = ChargeState::FULL;
        } else {
            charge_state_ = ChargeState::DISCHARGING;
        }
#endif
    } else {
        // Without hardware: derive charge state from voltage trend only.
        charge_state_ = ChargeState::DISCHARGING;
    }
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
        sample_battery();
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
