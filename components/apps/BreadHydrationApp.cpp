#include "apps/BreadHydrationApp.h"

#include <cstdio>
#include <cstdlib>
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
void recalc_event(lv_event_t* e) { static_cast<BreadHydrationApp*>(lv_event_get_user_data(e))->recalc(); }
}

void BreadHydrationApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); }

void BreadHydrationApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(row, ui::Spacing::LG, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, ui::Spacing::LG, 0);

    lv_obj_t* left = ui::create_card(row);
    lv_obj_set_size(left, 380, LV_PCT(100));
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(left, "Bread hydration");
    flour_ta_ = lv_textarea_create(left); lv_textarea_set_one_line(flour_ta_, true); lv_textarea_set_text(flour_ta_, "500");
    hydration_ta_ = lv_textarea_create(left); lv_textarea_set_one_line(hydration_ta_, true); lv_textarea_set_text(hydration_ta_, "72");
    ui::create_numpad(left, flour_ta_, false, false);
    lv_obj_add_event_cb(flour_ta_, recalc_event, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(hydration_ta_, recalc_event, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t* right = ui::create_card(row);
    lv_obj_set_size(right, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    result_label_ = lv_label_create(right);
    lv_obj_set_style_text_font(result_label_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(result_label_, lv_color_hex(ui::Color::GOLD_HI), 0);
    lv_label_set_text(result_label_, "0 g");
    const char* refs[] = {"55%  Very stiff", "65%  Sandwich dough", "72%  Country loaf", "80%  Open crumb", "100%  Very wet / focaccia"};
    for (const char* ref : refs) {
        lv_obj_t* lbl = lv_label_create(right);
        lv_label_set_text(lbl, ref);
    }
    recalc();
}

void BreadHydrationApp::recalc() {
    const double flour = std::atof(lv_textarea_get_text(flour_ta_));
    const double hydration = std::atof(lv_textarea_get_text(hydration_ta_));
    const double water = flour * hydration / 100.0;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.0f g water", water);
    lv_label_set_text(result_label_, buf);
}

void BreadHydrationApp::on_unmount() { root_ = nullptr; }
void BreadHydrationApp::on_update(float) {}

} // namespace apps
