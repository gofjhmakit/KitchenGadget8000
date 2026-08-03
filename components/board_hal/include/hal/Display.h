#pragma once
#include <cstdint>
#include "lvgl.h"

namespace hal {

struct DisplayConfig {
    static constexpr uint32_t WIDTH  = 1024;
    static constexpr uint32_t HEIGHT = 600;
    static constexpr int BACKLIGHT_GPIO = 31;
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
    void* io_handle_{nullptr};
    void* dsi_bus_{nullptr};
};

} // namespace hal
