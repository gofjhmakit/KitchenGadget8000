#pragma once
#include "lvgl.h"
#include <cstdint>

namespace ui {

namespace Color {
    static constexpr uint32_t BG          = 0x000000;
    static constexpr uint32_t SURFACE     = 0x0D0D0D;
    static constexpr uint32_t SURFACE_2   = 0x1A1A0D;
    static constexpr uint32_t SURFACE_3   = 0x2A2A1A;
    static constexpr uint32_t GOLD_HI     = 0xFFD166;
    static constexpr uint32_t GOLD        = 0xC9A84C;
    static constexpr uint32_t GOLD_DIM    = 0x7A6025;
    static constexpr uint32_t GOLD_FAINT  = 0x1C1200;
    static constexpr uint32_t GOLD_TRACK  = 0x151008;
    static constexpr uint32_t TEXT_PRI    = 0xFFFFFF;
    static constexpr uint32_t TEXT_SEC    = 0xB0A080;
    static constexpr uint32_t TEXT_HINT   = 0x665540;
    static constexpr uint32_t SEPARATOR   = 0x1E1A10;
    static constexpr uint32_t SUCCESS     = 0x4CAF50;
    static constexpr uint32_t WARNING     = 0xFFB74D;
    static constexpr uint32_t ERROR       = 0xEF5350;
    static constexpr uint32_t TIMER_1     = 0xFFD166;
    static constexpr uint32_t TIMER_2     = 0x80CBC4;
    static constexpr uint32_t TIMER_3     = 0xFFAB91;
    static constexpr uint32_t TIMER_4     = 0xCE93D8;
    static constexpr uint32_t TIMER_5     = 0x90CAF9;
}

namespace Spacing {
    static constexpr int XS  = 4;
    static constexpr int SM  = 8;
    static constexpr int MD  = 16;
    static constexpr int LG  = 24;
    static constexpr int XL  = 40;
    static constexpr int XXL = 64;
}

namespace Font {
    static constexpr int SIZE_HUGE    = 48;
    static constexpr int SIZE_TITLE   = 32;
    static constexpr int SIZE_HEADING = 24;
    static constexpr int SIZE_BODY    = 20;
    static constexpr int SIZE_LABEL   = 16;
    static constexpr int SIZE_SMALL   = 14;
}

namespace Radius {
    static constexpr int SM = 8;
    static constexpr int MD = 16;
    static constexpr int LG = 24;
    static constexpr int XL = 40;
}

class Theme {
public:
    static Theme& instance();
    void init();
    void apply();
    static const lv_font_t* font_huge();
    static const lv_font_t* font_title();
    static const lv_font_t* font_heading();
    static const lv_font_t* font_body();
    static const lv_font_t* font_body_bold() { return font_heading(); } // heading size used for bold body text
    static const lv_font_t* font_label();
    static const lv_font_t* font_small();

private:
    Theme() = default;
    lv_theme_t* theme_{nullptr};
};

lv_obj_t* create_gold_button(lv_obj_t* parent, const char* label);
lv_obj_t* create_card(lv_obj_t* parent);
lv_obj_t* create_section_title(lv_obj_t* parent, const char* text);
lv_obj_t* create_divider(lv_obj_t* parent);
void style_number_label(lv_obj_t* label, uint32_t color = Color::GOLD_HI);

} // namespace ui
