#include "apps/MeatTemperaturesApp.h"

#include <array>
#include <cstdio>
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
struct MeatType {
    const char* name;
    const char* detail;
    int c;
    const char* animal;
    const char* tip;
};
constexpr std::array<MeatType, 5> kMeats{{
    {"Beef",    "Medium Rare", 57, "🐄", "Rest 5–10 min for perfect results."},
    {"Pork",    "Minimum",     63, "🐷", "Let rest 3 minutes before serving."},
    {"Lamb",    "Medium Rare", 57, "🐑", "Rest 5 minutes for best flavour."},
    {"Poultry", "Minimum",     74, "🐔", "Always cook to minimum safe temp."},
    {"Fish",    "Minimum",     63, "🐟", "Fish is done when it flakes easily."},
}};

void select_meat(lv_event_t* e) {
    auto* app = static_cast<MeatTemperaturesApp*>(lv_event_get_user_data(e));
    const int idx = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    app->select(idx);
}
} // anonymous namespace

void MeatTemperaturesApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); }

void MeatTemperaturesApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* outer = lv_obj_create(parent);
    lv_obj_remove_style_all(outer);
    lv_obj_set_size(outer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(outer, ui::Spacing::LG, 0);
    lv_obj_set_layout(outer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(outer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(outer, ui::Spacing::MD, 0);

    ui::create_app_header(outer, "MEAT TEMPERATURES");

    lv_obj_t* body = lv_obj_create(outer);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(body, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(body, ui::Spacing::LG, 0);

    // ── Left: meat type list ──────────────────────────────────────────────────
    lv_obj_t* list_col = ui::create_card(body);
    lv_obj_set_size(list_col, 220, LV_PCT(100));
    lv_obj_set_layout(list_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list_col, ui::Spacing::SM, 0);

    for (int i = 0; i < static_cast<int>(kMeats.size()); ++i) {
        lv_obj_t* btn = lv_obj_create(list_col);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, LV_PCT(100), 56);
        lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(btn, ui::Spacing::MD, 0);
        lv_obj_set_style_radius(btn, ui::Radius::SM, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_0, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(ui::Color::GOLD_FAINT), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(ui::Color::GOLD_HI), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(btn, select_meat, LV_EVENT_CLICKED, this);
        meat_btns_[i] = btn;

        lv_obj_t* icon = lv_label_create(btn);
        lv_label_set_text(icon, kMeats[i].animal);
        lv_obj_set_style_text_font(icon, ui::Theme::font_body(), 0);
        lv_obj_set_width(icon, 32);

        lv_obj_t* name = lv_label_create(btn);
        lv_label_set_text(name, kMeats[i].name);
        lv_obj_set_style_text_font(name, ui::Theme::font_body(), 0);
        lv_obj_set_style_text_color(name, lv_color_hex(ui::Color::TEXT_PRI), 0);
    }

    // ── Right: detail panel ───────────────────────────────────────────────────
    detail_card_ = ui::create_card(body);
    lv_obj_set_size(detail_card_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(detail_card_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(detail_card_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(detail_card_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(detail_card_, ui::Spacing::SM, 0);

    detail_doneness_   = lv_label_create(detail_card_);
    detail_animal_     = lv_label_create(detail_card_);
    detail_temp_c_     = lv_label_create(detail_card_);
    detail_temp_f_     = lv_label_create(detail_card_);
    detail_tip_        = lv_label_create(detail_card_);

    lv_obj_set_style_text_font(detail_doneness_, ui::Theme::font_label(), 0);
    lv_obj_set_style_text_color(detail_doneness_, lv_color_hex(ui::Color::TEXT_SEC), 0);

    lv_obj_set_style_text_font(detail_animal_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(detail_animal_, lv_color_hex(ui::Color::GOLD), 0);

    ui::style_number_label(detail_temp_c_, ui::Color::GOLD_HI);
    lv_obj_set_style_text_font(detail_temp_c_, ui::Theme::font_huge(), 0);

    lv_obj_set_style_text_font(detail_temp_f_, ui::Theme::font_heading(), 0);
    lv_obj_set_style_text_color(detail_temp_f_, lv_color_hex(ui::Color::TEXT_SEC), 0);

    lv_obj_set_style_text_font(detail_tip_, ui::Theme::font_small(), 0);
    lv_obj_set_style_text_color(detail_tip_, lv_color_hex(ui::Color::TEXT_HINT), 0);
    lv_obj_set_width(detail_tip_, LV_PCT(90));
    lv_label_set_long_mode(detail_tip_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(detail_tip_, LV_TEXT_ALIGN_CENTER, 0);

    select(0); // Default: Beef
}

void MeatTemperaturesApp::select(int idx) {
    selected_ = idx;
    const auto& m = kMeats[idx];

    // Update button highlights
    for (int i = 0; i < static_cast<int>(kMeats.size()); ++i) {
        if (meat_btns_[i] == nullptr) continue;
        if (i == idx) {
            lv_obj_set_style_bg_opa(meat_btns_[i], LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(meat_btns_[i], 1, 0);
            // Name label (child 1) — gold
            lv_obj_t* name_lbl = lv_obj_get_child(meat_btns_[i], 1);
            if (name_lbl) lv_obj_set_style_text_color(name_lbl, lv_color_hex(ui::Color::GOLD_HI), 0);
        } else {
            lv_obj_set_style_bg_opa(meat_btns_[i], LV_OPA_0, 0);
            lv_obj_set_style_border_width(meat_btns_[i], 0, 0);
            lv_obj_t* name_lbl = lv_obj_get_child(meat_btns_[i], 1);
            if (name_lbl) lv_obj_set_style_text_color(name_lbl, lv_color_hex(ui::Color::TEXT_PRI), 0);
        }
    }

    // Update detail panel
    char label_buf[64];
    std::snprintf(label_buf, sizeof(label_buf), "%s – %s", m.name, m.detail);
    lv_label_set_text(detail_doneness_, label_buf);
    lv_label_set_text(detail_animal_, m.animal);

    char tc[16], tf[16];
    std::snprintf(tc, sizeof(tc), "%d°C", m.c);
    std::snprintf(tf, sizeof(tf), "%d°F", static_cast<int>(m.c * 9.0 / 5.0 + 32.0));
    lv_label_set_text(detail_temp_c_, tc);
    lv_label_set_text(detail_temp_f_, tf);
    lv_label_set_text(detail_tip_, m.tip);
}

void MeatTemperaturesApp::on_unmount() { root_ = nullptr; detail_card_ = nullptr; }
void MeatTemperaturesApp::on_update(float) {}

} // namespace apps

