#include "apps/IngredientScalerApp.h"

#include <cstdio>
#include <cstdlib>
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
void recalc_event(lv_event_t* e) { static_cast<IngredientScalerApp*>(lv_event_get_user_data(e))->recalc(); }
}

void IngredientScalerApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); }

void IngredientScalerApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(row, ui::Spacing::LG, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, ui::Spacing::LG, 0);

    lv_obj_t* left = ui::create_card(row);
    lv_obj_set_size(left, 360, LV_PCT(100));
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(left, "Ingredient scaler");

    amount_ta_ = lv_textarea_create(left); lv_textarea_set_one_line(amount_ta_, true); lv_textarea_set_placeholder_text(amount_ta_, "Amount");
    servings_orig_ta_ = lv_textarea_create(left); lv_textarea_set_one_line(servings_orig_ta_, true); lv_textarea_set_placeholder_text(servings_orig_ta_, "Original servings"); lv_textarea_set_text(servings_orig_ta_, "4");
    servings_target_ta_ = lv_textarea_create(left); lv_textarea_set_one_line(servings_target_ta_, true); lv_textarea_set_placeholder_text(servings_target_ta_, "Target servings"); lv_textarea_set_text(servings_target_ta_, "6");
    unit_dd_ = lv_dropdown_create(left); lv_dropdown_set_options(unit_dd_, "g\nkg\nml\nL\ntsp\ntbsp\ncup\npiece");
    for (auto* ta : {amount_ta_, servings_orig_ta_, servings_target_ta_}) {
        lv_obj_set_width(ta, LV_PCT(100));
        lv_obj_set_style_text_font(ta, ui::Theme::font_body(), 0);
        lv_obj_add_event_cb(ta, recalc_event, LV_EVENT_VALUE_CHANGED, this);
    }
    lv_obj_add_event_cb(unit_dd_, recalc_event, LV_EVENT_VALUE_CHANGED, this);
    ui::create_numpad(left, amount_ta_, true, false);

    lv_obj_t* right = ui::create_card(row);
    lv_obj_set_size(right, LV_PCT(100), LV_PCT(100));
    result_label_ = lv_label_create(right);
    lv_obj_center(result_label_);
    lv_obj_set_style_text_font(result_label_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(result_label_, lv_color_hex(ui::Color::GOLD_HI), 0);
    recalc();
}

void IngredientScalerApp::recalc() {
    const double amount = std::atof(lv_textarea_get_text(amount_ta_));
    const double orig = std::max(1.0, std::atof(lv_textarea_get_text(servings_orig_ta_)));
    const double target = std::max(1.0, std::atof(lv_textarea_get_text(servings_target_ta_)));
    const double scaled = amount * (target / orig);
    char unit[16];
    lv_dropdown_get_selected_str(unit_dd_, unit, sizeof(unit));
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f %s", scaled, unit);
    lv_label_set_text(result_label_, buf);
}

void IngredientScalerApp::on_unmount() { root_ = nullptr; }
void IngredientScalerApp::on_update(float) {}

} // namespace apps
