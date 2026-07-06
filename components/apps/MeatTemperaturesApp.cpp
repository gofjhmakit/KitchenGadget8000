#include "apps/MeatTemperaturesApp.h"

#include <array>
#include <cstdio>
#include "ui/Theme.h"

namespace apps {
namespace {
struct Row { const char* name; const char* detail; int c; uint32_t color; };
constexpr std::array<Row, 9> kRows{{
    {"Beef", "Rare", 52, ui::Color::WARNING},
    {"Beef", "Medium-rare", 57, ui::Color::SUCCESS},
    {"Beef", "Medium", 63, ui::Color::SUCCESS},
    {"Beef", "Well", 74, ui::Color::ERROR},
    {"Chicken", "Minimum", 74, ui::Color::ERROR},
    {"Pork", "Minimum", 63, ui::Color::SUCCESS},
    {"Fish", "Minimum", 63, ui::Color::SUCCESS},
    {"Lamb", "Medium-rare", 57, ui::Color::SUCCESS},
    {"Ground meat", "Minimum", 71, ui::Color::ERROR},
}};
}

void MeatTemperaturesApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); }

void MeatTemperaturesApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont, ui::Spacing::LG, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(cont, "Safe meat temperatures");

    lv_obj_t* header = ui::create_card(cont);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    for (const char* h : {"Food", "Doneness", "°C", "°F"}) {
        lv_obj_t* lbl = lv_label_create(header);
        lv_obj_set_width(lbl, 220);
        lv_label_set_text(lbl, h);
        lv_obj_set_style_text_color(lbl, lv_color_hex(ui::Color::GOLD_HI), 0);
    }
    for (const auto& row : kRows) {
        lv_obj_t* item = ui::create_card(cont);
        lv_obj_set_width(item, LV_PCT(100));
        lv_obj_set_layout(item, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_border_color(item, lv_color_hex(row.color), 0);
        char fbuf[16];
        std::snprintf(fbuf, sizeof(fbuf), "%d", static_cast<int>(row.c * 9.0 / 5.0 + 32.0));
        const char* values[] = {row.name, row.detail, nullptr, nullptr};
        char cbuf[16]; std::snprintf(cbuf, sizeof(cbuf), "%d", row.c);
        for (int i = 0; i < 4; ++i) {
            lv_obj_t* lbl = lv_label_create(item);
            lv_obj_set_width(lbl, 220);
            lv_label_set_text(lbl, i == 2 ? cbuf : i == 3 ? fbuf : values[i]);
            if (i >= 2) lv_obj_set_style_text_color(lbl, lv_color_hex(row.color), 0);
        }
    }
}

void MeatTemperaturesApp::on_unmount() { root_ = nullptr; }
void MeatTemperaturesApp::on_update(float) {}

} // namespace apps
