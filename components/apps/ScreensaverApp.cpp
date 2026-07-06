#include "apps/ScreensaverApp.h"

#include <cstdio>
#include "core/Navigation.h"
#include "core/PowerManager.h"
#include "services/TimeService.h"
#include "ui/Animations.h"
#include "ui/Theme.h"

namespace apps {
namespace {
void wake_event(lv_event_t* e) {
    core::PowerManager::instance().reset_activity();
    core::Navigation::instance().go_home();
    (void)e;
}
}

void ScreensaverApp::on_mount(lv_obj_t* parent) {
    root_ = parent;
    build_ui(parent);
}

void ScreensaverApp::build_ui(lv_obj_t* parent) {
    lv_obj_set_style_bg_color(parent, lv_color_hex(ui::Color::BG), 0);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(parent, wake_event, LV_EVENT_CLICKED, nullptr);

    pulse_circle_ = lv_obj_create(parent);
    lv_obj_remove_style_all(pulse_circle_);
    lv_obj_set_size(pulse_circle_, 320, 320);
    lv_obj_center(pulse_circle_);
    lv_obj_set_style_radius(pulse_circle_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(pulse_circle_, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_border_width(pulse_circle_, 2, 0);
    lv_obj_set_style_bg_opa(pulse_circle_, LV_OPA_TRANSP, 0);
    ui::anim::pulse_glow(pulse_circle_);

    lv_obj_t* col = lv_obj_create(parent);
    lv_obj_remove_style_all(col);
    lv_obj_center(col);
    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    clock_label_ = lv_label_create(col);
    lv_obj_set_style_text_font(clock_label_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(clock_label_, lv_color_hex(ui::Color::GOLD_HI), 0);
    date_label_ = lv_label_create(col);
    lv_obj_set_style_text_font(date_label_, ui::Theme::font_heading(), 0);
    lv_obj_set_style_text_color(date_label_, lv_color_hex(ui::Color::TEXT_SEC), 0);
    day_label_ = lv_label_create(col);
    lv_obj_set_style_text_font(day_label_, ui::Theme::font_body(), 0);
    lv_obj_set_style_text_color(day_label_, lv_color_hex(ui::Color::GOLD), 0);
    battery_label_ = lv_label_create(parent);
    lv_obj_align(battery_label_, LV_ALIGN_BOTTOM_RIGHT, -24, -18);
    lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::TEXT_HINT), 0);
    on_update(0.0f);
}

void ScreensaverApp::on_unmount() {
    if (pulse_circle_) {
        ui::anim::stop_pulse(pulse_circle_);
    }
    root_ = nullptr;
}

void ScreensaverApp::on_update(float) {
    if (!root_) return;
    lv_label_set_text(clock_label_, services::TimeService::instance().time_string(false).c_str());
    lv_label_set_text(date_label_, services::TimeService::instance().date_string().c_str());
    lv_label_set_text(day_label_, services::TimeService::instance().day_name().c_str());
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%u%%", core::PowerManager::instance().battery_percent());
    lv_label_set_text(battery_label_, buf);
}

} // namespace apps
