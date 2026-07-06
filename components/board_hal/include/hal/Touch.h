#pragma once
#include <cstdint>
#include "lvgl.h"

namespace hal {

struct TouchConfig {
    static constexpr int I2C_PORT    = 0;
    static constexpr int SCL_GPIO    = 8;
    static constexpr int SDA_GPIO    = 7;
    static constexpr int INT_GPIO    = 4;
    static constexpr int RST_GPIO    = 38;
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
