#include "apps/ConverterApp.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
struct Unit { const char* name; double to_base; };
// All units normalised: Volume base = ml, Weight base = g, Temp base = °C
constexpr std::array<Unit, 9> kVolume{{
    {"ml",1.0},{"L",1000.0},{"tsp",4.929},{"tbsp",14.787},
    {"fl oz",29.574},{"cup",236.588},{"pint",473.176},{"quart",946.353},{"gallon",3785.41}}};
constexpr std::array<Unit, 4> kWeight{{{ "g",1.0},{"kg",1000.0},{"oz",28.3495},{"lb",453.592}}};
// Quick-chip labels and FROM unit indices they map to (into the combined set)
// [Volume: 0-8, Weight: 9-12, Temp: 13-15]
constexpr std::array<const char*, 8> kChipLabels{{"tsp","tbsp","oz","ml","g","kg","°C","°F"}};
constexpr std::array<int, 8>         kChipFrom{{2, 3, 11, 0, 9, 10, 13, 14}};

struct DropdownItem { const char* label; int category; int unit_idx; };
// Flattened list: [0-8 = Volume], [9-12 = Weight], [13 = °C, 14 = °F, 15 = K]
const char* unit_name(int flat_idx) {
    if (flat_idx < 9) return kVolume[flat_idx].name;
    if (flat_idx < 13) return kWeight[flat_idx - 9].name;
    static const char* temps[] = {"°C","°F","K"};
    return temps[flat_idx - 13];
}

double convert(double value, int from_idx, int to_idx) {
    // Temperature special-case
    const bool from_temp = (from_idx >= 13);
    const bool to_temp   = (to_idx   >= 13);
    if (from_temp || to_temp) {
        double celsius = value;
        if (from_idx == 14) celsius = (value - 32.0) * 5.0 / 9.0;
        else if (from_idx == 15) celsius = value - 273.15;
        if (to_idx == 13) return celsius;
        if (to_idx == 14) return celsius * 9.0 / 5.0 + 32.0;
        if (to_idx == 15) return celsius + 273.15;
        return celsius;
    }
    // Volume
    if (from_idx < 9 && to_idx < 9) {
        return value * kVolume[from_idx].to_base / kVolume[to_idx].to_base;
    }
    // Weight
    if (from_idx >= 9 && from_idx < 13 && to_idx >= 9 && to_idx < 13) {
        return value * kWeight[from_idx - 9].to_base / kWeight[to_idx - 9].to_base;
    }
    return value; // cross-category: no conversion
}

std::string make_dropdown_options() {
    std::string s;
    for (int i = 0; i < 16; ++i) {
        if (!s.empty()) s += '\n';
        s += unit_name(i);
    }
    return s;
}

void input_tap(lv_event_t* e) {
    auto* app = static_cast<ConverterApp*>(lv_event_get_user_data(e));
    ui::show_numpad_modal(app->root_, app->input_ta_, true, true);
}
void dropdown_changed(lv_event_t* e) {
    static_cast<ConverterApp*>(lv_event_get_user_data(e))->update_results();
}
void swap_units(lv_event_t* e) {
    auto* app = static_cast<ConverterApp*>(lv_event_get_user_data(e));
    const uint32_t from_sel = lv_dropdown_get_selected(app->from_dd_);
    const uint32_t to_sel   = lv_dropdown_get_selected(app->to_dd_);
    lv_dropdown_set_selected(app->from_dd_, to_sel);
    lv_dropdown_set_selected(app->to_dd_,   from_sel);
    app->update_results();
}
void chip_click(lv_event_t* e) {
    auto* app = static_cast<ConverterApp*>(lv_event_get_user_data(e));
    const int chip_idx = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    lv_dropdown_set_selected(app->from_dd_, static_cast<uint32_t>(kChipFrom[chip_idx]));
    app->update_results();
}
} // anonymous namespace

void ConverterApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); }

void ConverterApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* outer = lv_obj_create(parent);
    lv_obj_remove_style_all(outer);
    lv_obj_set_size(outer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(outer, ui::Spacing::LG, 0);
    lv_obj_set_layout(outer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(outer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(outer, ui::Spacing::MD, 0);

    ui::create_app_header(outer, "UNIT CONVERTER");

    const std::string opts = make_dropdown_options();

    // ── FROM / SWAP / TO row ─────────────────────────────────────────────────
    lv_obj_t* sel_row = lv_obj_create(outer);
    lv_obj_remove_style_all(sel_row);
    lv_obj_set_size(sel_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(sel_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sel_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sel_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(sel_row, ui::Spacing::SM, 0);

    auto make_dd_label = [](lv_obj_t* parent, const char* text) {
        lv_obj_t* lbl = lv_label_create(parent);
        lv_label_set_text(lbl, text);
        lv_obj_set_style_text_font(lbl, ui::Theme::font_small(), 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(ui::Color::TEXT_HINT), 0);
    };

    lv_obj_t* from_col = lv_obj_create(sel_row);
    lv_obj_remove_style_all(from_col);
    lv_obj_set_flex_grow(from_col, 1);
    lv_obj_set_layout(from_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(from_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(from_col, ui::Spacing::XS, 0);
    make_dd_label(from_col, "FROM");
    from_dd_ = lv_dropdown_create(from_col);
    lv_dropdown_set_options(from_dd_, opts.c_str());
    lv_obj_set_width(from_dd_, LV_PCT(100));
    lv_obj_set_style_bg_color(from_dd_, lv_color_hex(ui::Color::SURFACE_2), 0);
    lv_obj_set_style_border_color(from_dd_, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_text_color(from_dd_, lv_color_hex(ui::Color::TEXT_PRI), 0);
    lv_obj_add_event_cb(from_dd_, dropdown_changed, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t* swap_btn = ui::create_gold_button(sel_row, "⇄");
    lv_obj_set_style_pad_hor(swap_btn, ui::Spacing::LG, 0);
    lv_obj_add_event_cb(swap_btn, swap_units, LV_EVENT_CLICKED, this);

    lv_obj_t* to_col = lv_obj_create(sel_row);
    lv_obj_remove_style_all(to_col);
    lv_obj_set_flex_grow(to_col, 1);
    lv_obj_set_layout(to_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(to_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(to_col, ui::Spacing::XS, 0);
    make_dd_label(to_col, "TO");
    to_dd_ = lv_dropdown_create(to_col);
    lv_dropdown_set_options(to_dd_, opts.c_str());
    lv_dropdown_set_selected(to_dd_, 1); // default: L
    lv_obj_set_width(to_dd_, LV_PCT(100));
    lv_obj_set_style_bg_color(to_dd_, lv_color_hex(ui::Color::SURFACE_2), 0);
    lv_obj_set_style_border_color(to_dd_, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_text_color(to_dd_, lv_color_hex(ui::Color::TEXT_PRI), 0);
    lv_obj_add_event_cb(to_dd_, dropdown_changed, LV_EVENT_VALUE_CHANGED, this);

    // ── Value display row ─────────────────────────────────────────────────────
    lv_obj_t* val_row = lv_obj_create(outer);
    lv_obj_remove_style_all(val_row);
    lv_obj_set_size(val_row, LV_PCT(100), 140);
    lv_obj_set_layout(val_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(val_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(val_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(val_row, ui::Spacing::SM, 0);

    // FROM value card (tappable → opens numpad modal)
    lv_obj_t* from_card = ui::create_card(val_row);
    lv_obj_set_flex_grow(from_card, 1);
    lv_obj_set_style_min_height(from_card, 120, 0);
    lv_obj_set_layout(from_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(from_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(from_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(from_card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_color(from_card, lv_color_hex(ui::Color::GOLD_DIM), LV_STATE_PRESSED);
    lv_obj_add_event_cb(from_card, input_tap, LV_EVENT_CLICKED, this);

    // Hidden textarea used as value store
    input_ta_ = lv_textarea_create(from_card);
    lv_obj_add_flag(input_ta_, LV_OBJ_FLAG_HIDDEN);
    lv_textarea_set_text(input_ta_, "1");
    lv_obj_add_event_cb(input_ta_, [](lv_event_t* e){ static_cast<ConverterApp*>(lv_event_get_user_data(e))->update_results(); }, LV_EVENT_VALUE_CHANGED, this);

    from_display_ = lv_label_create(from_card);
    lv_label_set_text(from_display_, "1");
    lv_obj_set_style_text_font(from_display_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(from_display_, lv_color_hex(ui::Color::GOLD_HI), 0);

    from_unit_display_ = lv_label_create(from_card);
    lv_obj_set_style_text_font(from_unit_display_, ui::Theme::font_body(), 0);
    lv_obj_set_style_text_color(from_unit_display_, lv_color_hex(ui::Color::TEXT_SEC), 0);

    lv_obj_t* arrow_lbl = lv_label_create(val_row);
    lv_label_set_text(arrow_lbl, "=");
    lv_obj_set_style_text_font(arrow_lbl, ui::Theme::font_heading(), 0);
    lv_obj_set_style_text_color(arrow_lbl, lv_color_hex(ui::Color::GOLD_DIM), 0);

    // TO value card (read-only)
    lv_obj_t* to_card = ui::create_card(val_row);
    lv_obj_set_flex_grow(to_card, 1);
    lv_obj_set_style_min_height(to_card, 120, 0);
    lv_obj_set_layout(to_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(to_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(to_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    result_label_ = lv_label_create(to_card);
    lv_obj_set_style_text_font(result_label_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(result_label_, lv_color_hex(ui::Color::GOLD_HI), 0);

    to_unit_display_ = lv_label_create(to_card);
    lv_obj_set_style_text_font(to_unit_display_, ui::Theme::font_body(), 0);
    lv_obj_set_style_text_color(to_unit_display_, lv_color_hex(ui::Color::TEXT_SEC), 0);

    // ── Quick convert chips ───────────────────────────────────────────────────
    lv_obj_t* chip_label = lv_label_create(outer);
    lv_label_set_text(chip_label, "Quick convert");
    lv_obj_set_style_text_font(chip_label, ui::Theme::font_small(), 0);
    lv_obj_set_style_text_color(chip_label, lv_color_hex(ui::Color::TEXT_HINT), 0);

    lv_obj_t* chips = lv_obj_create(outer);
    lv_obj_remove_style_all(chips);
    lv_obj_set_size(chips, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(chips, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(chips, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(chips, ui::Spacing::SM, 0);
    for (int i = 0; i < static_cast<int>(kChipLabels.size()); ++i) {
        lv_obj_t* chip = ui::create_gold_button(chips, kChipLabels[i]);
        lv_obj_set_style_pad_hor(chip, ui::Spacing::MD, 0);
        lv_obj_set_style_pad_ver(chip, ui::Spacing::SM, 0);
        lv_obj_set_user_data(chip, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(chip, chip_click, LV_EVENT_CLICKED, this);
    }

    update_results();
}

void ConverterApp::update_results() {
    if (!result_label_ || !input_ta_) return;

    const double value = std::atof(lv_textarea_get_text(input_ta_));
    const int from_idx = static_cast<int>(lv_dropdown_get_selected(from_dd_));
    const int to_idx   = static_cast<int>(lv_dropdown_get_selected(to_dd_));
    const double result = convert(value, from_idx, to_idx);

    // Update FROM display
    char fbuf[32];
    if (value == static_cast<int>(value)) std::snprintf(fbuf, sizeof(fbuf), "%.0f", value);
    else std::snprintf(fbuf, sizeof(fbuf), "%.3g", value);
    lv_label_set_text(from_display_, fbuf);
    lv_label_set_text(from_unit_display_, unit_name(from_idx));

    // Update result display
    char rbuf[32];
    if (result == static_cast<long long>(result) && result < 1e9)
        std::snprintf(rbuf, sizeof(rbuf), "%.0f", result);
    else
        std::snprintf(rbuf, sizeof(rbuf), "%.4g", result);
    lv_label_set_text(result_label_, rbuf);
    lv_label_set_text(to_unit_display_, unit_name(to_idx));
}

void ConverterApp::on_unmount() { root_ = nullptr; }
void ConverterApp::on_update(float) {}

} // namespace apps

