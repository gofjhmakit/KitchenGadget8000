#pragma once
#include <cstdint>
#include "lvgl.h"

namespace hal {

struct DisplayConfig {
    static constexpr uint32_t WIDTH  = 1024;
    static constexpr uint32_t HEIGHT = 600;
    static constexpr uint32_t PCLK_HZ = 18000000;
    static constexpr int HSYNC_GPIO = 46;
    static constexpr int VSYNC_GPIO = 3;
    static constexpr int DE_GPIO    = 17;
    static constexpr int PCLK_GPIO  = 9;
    static constexpr int R0_GPIO = 10, R1_GPIO = 11, R2_GPIO = 12, R3_GPIO = 13, R4_GPIO = 14;
    static constexpr int G0_GPIO = 21, G1_GPIO = 47, G2_GPIO = 48, G3_GPIO = 45, G4_GPIO = 38, G5_GPIO = 39;
    static constexpr int B0_GPIO = 40, B1_GPIO = 41, B2_GPIO = 42, B3_GPIO = 2,  B4_GPIO = 1;
    static constexpr int BACKLIGHT_GPIO = 0;
    static constexpr int LCD_RST_GPIO   = -1;
};

class Display {
public:
    static Display& instance();
    bool init();
    void set_backlight(uint8_t brightness);
    uint32_t width() const  { return DisplayConfig::WIDTH; }
    uint32_t height() const { return DisplayConfig::HEIGHT; }
    lv_display_t* get_lv_display() const { return lv_disp_; }

private:
    Display() = default;
    static void flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* color_map);
    lv_display_t* lv_disp_{nullptr};
    void* panel_handle_{nullptr};
};

} // namespace hal
