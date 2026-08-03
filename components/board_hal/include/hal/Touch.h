#pragma once
#include <cstdint>
#include "lvgl.h"

namespace hal {

struct TouchConfig {
    static constexpr int I2C_PORT    = 0;
    // Elecrow CrowPanel Advanced 9" ESP32-P4: I2C on GPIO45(SDA)/GPIO46(SCL)
    static constexpr int SCL_GPIO    = 46;
    static constexpr int SDA_GPIO    = 45;
    // GT911 reset/interrupt from Elecrow bsp_display.h
    static constexpr int RST_GPIO    = 40;
    static constexpr int INT_GPIO    = 42;
    static constexpr uint8_t GT911_ADDR = 0x5D;
    static constexpr uint32_t I2C_FREQ  = 400000;
};

class Touch {
public:
    static Touch& instance();
    bool init();
    lv_indev_t* get_lv_indev() const { return lv_indev_; }

private:
    Touch() = default;
    static void read_cb(lv_indev_t* indev, lv_indev_data_t* data);
    lv_indev_t* lv_indev_{nullptr};
    void* i2c_dev_{nullptr};
};

} // namespace hal
