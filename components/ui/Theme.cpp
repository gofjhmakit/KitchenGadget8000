#include "ui/Theme.h"

#include "hal/Display.h"

namespace ui {
namespace {
constexpr lv_coord_t PAD = 16;
}

Theme& Theme::instance() {
    static Theme inst;
    return inst;
}

const lv_font_t* Theme::font_huge() { return &lv_font_montserrat_48; }
const lv_font_t* Theme::font_title() { return &lv_font_montserrat_32; }
const lv_font_t* Theme::font_heading() { return &lv_font_montserrat_24; }
const lv_font_t* Theme::font_body() { return &lv_font_montserrat_20; }
const lv_font_t* Theme::font_label() { return &lv_font_montserrat_16; }
const lv_font_t* Theme::font_small() { return &lv_font_montserrat_14; }

void Theme::init() {
    auto* display = hal::Display::instance().get_lv_display();
    theme_ = lv_theme_default_init(display,
                                   lv_color_hex(Color::GOLD_HI),
                                   lv_color_hex(Color::GOLD),
                                   true,
                                   font_body());
    lv_display_set_theme(display, theme_);
}

void Theme::apply() {
    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(Color::BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(Color::TEXT_PRI), 0);
    lv_obj_set_style_text_font(screen, font_body(), 0);
    lv_obj_set_style_pad_all(screen, PAD, 0);
}

} // namespace ui
