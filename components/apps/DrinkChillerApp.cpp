#include "apps/DrinkChillerApp.h"

#include <cmath>
#include <cstdio>
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
void recalc_event(lv_event_t* e) { static_cast<DrinkChillerApp*>(lv_event_get_user_data(e))->recalc(); }
}

void DrinkChillerApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); }

void DrinkChillerApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(row, ui::Spacing::LG, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, ui::Spacing::LG, 0);

    lv_obj_t* left = ui::create_card(row);
    lv_obj_set_size(left, 420, LV_PCT(100));
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(left, "Drink chiller");
    container_dd_ = lv_dropdown_create(left); lv_dropdown_set_options(container_dd_, "Can 330ml\nBottle 500ml\nLarge bottle 1.5L\nWine bottle 750ml");
    start_value_ = lv_label_create(left);
    start_slider_ = lv_slider_create(left); lv_slider_set_range(start_slider_, 5, 30); lv_slider_set_value(start_slider_, 20, LV_ANIM_OFF);
    target_value_ = lv_label_create(left);
    target_slider_ = lv_slider_create(left); lv_slider_set_range(target_slider_, 2, 12); lv_slider_set_value(target_slider_, 6, LV_ANIM_OFF);
    location_dd_ = lv_dropdown_create(left); lv_dropdown_set_options(location_dd_, "Freezer -18°C\nFridge 4°C");
    for (auto* obj : {container_dd_, start_slider_, target_slider_, location_dd_}) {
        lv_obj_add_event_cb(obj, recalc_event, LV_EVENT_VALUE_CHANGED, this);
    }

    lv_obj_t* right = ui::create_card(row);
    lv_obj_set_size(right, LV_PCT(100), LV_PCT(100));
    result_label_ = lv_label_create(right);
    lv_obj_center(result_label_);
    lv_obj_set_style_text_font(result_label_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(result_label_, lv_color_hex(ui::Color::GOLD_HI), 0);
    recalc();
}

void DrinkChillerApp::recalc() {
    const int start = lv_slider_get_value(start_slider_);
    const int target = lv_slider_get_value(target_slider_);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Start %d°C", start);
    lv_label_set_text(start_value_, buf);
    std::snprintf(buf, sizeof(buf), "Target %d°C", target);
    lv_label_set_text(target_value_, buf);

    const int container = lv_dropdown_get_selected(container_dd_);
    const int location = lv_dropdown_get_selected(location_dd_);
    const double env = location == 0 ? -18.0 : 4.0;
    const double k[] = {0.055, 0.043, 0.018, 0.032};
    const double numerator = target - env;
    const double denominator = start - env;
    double minutes = 0.0;
    if (numerator > 0.0 && denominator > numerator) {
        minutes = -std::log(numerator / denominator) / k[container];
    }
    std::snprintf(buf, sizeof(buf), "%.0f min", minutes);
    lv_label_set_text(result_label_, buf);
}

void DrinkChillerApp::on_unmount() { root_ = nullptr; }
void DrinkChillerApp::on_update(float) {}

} // namespace apps
