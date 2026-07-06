#include "apps/ConverterApp.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
struct Unit { const char* name; double factor; };
constexpr std::array<Unit, 9> kVolume{{{"ml",1.0},{"L",0.001},{"tsp",0.202884},{"tbsp",0.067628},{"fl oz",0.033814},{"cup",0.00422675},{"pint",0.00211338},{"quart",0.00105669},{"gallon",0.000264172}}};
constexpr std::array<Unit, 4> kWeight{{{"g",1.0},{"kg",0.001},{"oz",0.035274},{"lb",0.00220462}}};

void category_select(lv_event_t* e) {
    auto* app = static_cast<ConverterApp*>(lv_event_get_user_data(e));
    app->category_ = static_cast<ConverterApp::Category>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    app->update_results();
}

void input_changed(lv_event_t* e) {
    static_cast<ConverterApp*>(lv_event_get_user_data(e))->update_results();
}
}

void ConverterApp::on_mount(lv_obj_t* parent) {
    root_ = parent;
    build_ui(parent);
}

void ConverterApp::build_ui(lv_obj_t* parent) {
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
    ui::create_section_title(left, "Converter");

    lv_obj_t* tabs = lv_obj_create(left);
    lv_obj_remove_style_all(tabs);
    lv_obj_set_width(tabs, LV_PCT(100));
    lv_obj_set_layout(tabs, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(tabs, ui::Spacing::SM, 0);
    const char* names[] = {"Volume", "Weight", "Temp"};
    for (int i = 0; i < 3; ++i) {
        lv_obj_t* btn = ui::create_gold_button(tabs, names[i]);
        lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(btn, category_select, LV_EVENT_CLICKED, this);
    }

    input_ta_ = lv_textarea_create(left);
    lv_obj_set_width(input_ta_, LV_PCT(100));
    lv_textarea_set_one_line(input_ta_, true);
    lv_textarea_set_placeholder_text(input_ta_, "Enter value (ml, g, or °C base)");
    lv_obj_set_style_text_font(input_ta_, ui::Theme::font_title(), 0);
    lv_obj_add_event_cb(input_ta_, input_changed, LV_EVENT_VALUE_CHANGED, this);
    ui::create_numpad(left, input_ta_, true, true);

    result_container_ = ui::create_card(row);
    lv_obj_set_size(result_container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(result_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(result_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(result_container_, ui::Spacing::SM, 0);
    update_results();
}

void ConverterApp::update_results() {
    if (!result_container_) return;
    lv_obj_clean(result_container_);
    ui::create_section_title(result_container_, category_ == Category::VOLUME ? "Volume" : category_ == Category::WEIGHT ? "Weight" : "Temperature");
    const double input = std::atof(lv_textarea_get_text(input_ta_));
    char line[96];
    if (category_ == Category::VOLUME) {
        for (const auto& unit : kVolume) {
            std::snprintf(line, sizeof(line), "%s  %.2f", unit.name, input * unit.factor);
            lv_obj_t* lbl = lv_label_create(result_container_);
            lv_label_set_text(lbl, line);
        }
    } else if (category_ == Category::WEIGHT) {
        for (const auto& unit : kWeight) {
            std::snprintf(line, sizeof(line), "%s  %.2f", unit.name, input * unit.factor);
            lv_obj_t* lbl = lv_label_create(result_container_);
            lv_label_set_text(lbl, line);
        }
    } else {
        const double c = input;
        const double f = c * 9.0 / 5.0 + 32.0;
        const double k = c + 273.15;
        const char* labels[] = {"°C", "°F", "K"};
        const double vals[] = {c, f, k};
        for (int i = 0; i < 3; ++i) {
            std::snprintf(line, sizeof(line), "%s  %.2f", labels[i], vals[i]);
            lv_obj_t* lbl = lv_label_create(result_container_);
            lv_label_set_text(lbl, line);
        }
    }
}

void ConverterApp::on_unmount() { root_ = nullptr; }
void ConverterApp::on_update(float) {}

} // namespace apps
