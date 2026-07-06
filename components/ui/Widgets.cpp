#include "ui/Widgets.h"

#include <array>
#include <cstdio>
#include <cstring>
#include "ui/Theme.h"

namespace ui {
namespace {
constexpr const char* CHEVRON = "›";

void keypad_event(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target_obj(e);
    lv_obj_t* ta = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    const char* txt = lv_label_get_text(label);
    if (std::strcmp(txt, "←") == 0) {
        lv_textarea_delete_char(ta);
    } else if (std::strcmp(txt, "C") == 0) {
        lv_textarea_set_text(ta, "");
    } else {
        lv_textarea_add_text(ta, txt);
    }
    lv_event_send(ta, LV_EVENT_VALUE_CHANGED, nullptr);
}
}

lv_obj_t* create_gold_button(lv_obj_t* parent, const char* label) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_style_bg_color(btn, lv_color_hex(Color::GOLD_FAINT), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(Color::GOLD_DIM), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(Color::GOLD), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_radius(btn, Radius::MD, 0);
    lv_obj_set_style_pad_hor(btn, Spacing::LG, 0);
    lv_obj_set_style_pad_ver(btn, Spacing::MD, 0);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, lv_color_hex(Color::GOLD_HI), 0);
    lv_obj_set_style_text_font(lbl, Theme::font_body(), 0);
    lv_obj_center(lbl);
    return btn;
}

lv_obj_t* create_card(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(Color::SURFACE), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(Color::SEPARATOR), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, Radius::LG, 0);
    lv_obj_set_style_pad_all(card, Spacing::MD, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(Color::GOLD_DIM), 0);
    lv_obj_set_style_shadow_width(card, 12, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    return card;
}

lv_obj_t* create_section_title(lv_obj_t* parent, const char* text) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(Color::GOLD_HI), 0);
    lv_obj_set_style_text_font(label, Theme::font_heading(), 0);
    return label;
}

lv_obj_t* create_divider(lv_obj_t* parent) {
    lv_obj_t* line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_size(line, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(line, lv_color_hex(Color::SEPARATOR), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    return line;
}

void style_number_label(lv_obj_t* label, uint32_t color) {
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label, Theme::font_title(), 0);
}

lv_obj_t* create_progress_ring(lv_obj_t* parent, uint32_t color, int size) {
    lv_obj_t* arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(Color::GOLD_TRACK), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

lv_obj_t* create_stat_card(lv_obj_t* parent, const char* title, const char* value, const char* subtitle, const char* icon) {
    lv_obj_t* card = create_card(parent);
    lv_obj_set_size(card, 240, LV_SIZE_CONTENT);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    if (icon != nullptr) {
        lv_obj_t* icon_lbl = lv_label_create(card);
        lv_label_set_text(icon_lbl, icon);
        lv_obj_set_style_text_font(icon_lbl, Theme::font_heading(), 0);
        lv_obj_set_style_text_color(icon_lbl, lv_color_hex(Color::GOLD_HI), 0);
    }
    lv_obj_t* title_lbl = lv_label_create(card);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(Color::TEXT_SEC), 0);
    lv_obj_t* value_lbl = lv_label_create(card);
    lv_label_set_text(value_lbl, value);
    style_number_label(value_lbl, Color::GOLD_HI);
    if (subtitle != nullptr) {
        lv_obj_t* sub_lbl = lv_label_create(card);
        lv_label_set_text(sub_lbl, subtitle);
        lv_obj_set_style_text_color(sub_lbl, lv_color_hex(Color::TEXT_HINT), 0);
        lv_obj_set_style_text_font(sub_lbl, Theme::font_small(), 0);
    }
    return card;
}

lv_obj_t* create_list_item(lv_obj_t* parent, const char* title, const char* subtitle, const char* icon) {
    lv_obj_t* btn = create_card(parent);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_border_color(btn, lv_color_hex(Color::GOLD_DIM), LV_STATE_PRESSED);

    if (icon != nullptr) {
        lv_obj_t* icon_lbl = lv_label_create(btn);
        lv_label_set_text(icon_lbl, icon);
        lv_obj_set_width(icon_lbl, 48);
        lv_obj_set_style_text_font(icon_lbl, Theme::font_heading(), 0);
    }

    lv_obj_t* text_col = lv_obj_create(btn);
    lv_obj_remove_style_all(text_col);
    lv_obj_set_flex_grow(text_col, 1);
    lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* title_lbl = lv_label_create(text_col);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_font(title_lbl, Theme::font_body(), 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(Color::TEXT_PRI), 0);

    if (subtitle != nullptr) {
        lv_obj_t* sub_lbl = lv_label_create(text_col);
        lv_label_set_text(sub_lbl, subtitle);
        lv_obj_set_style_text_color(sub_lbl, lv_color_hex(Color::TEXT_SEC), 0);
        lv_obj_set_style_text_font(sub_lbl, Theme::font_small(), 0);
    }

    lv_obj_t* chev = lv_label_create(btn);
    lv_label_set_text(chev, CHEVRON);
    lv_obj_set_style_text_color(chev, lv_color_hex(Color::GOLD), 0);
    lv_obj_set_style_text_font(chev, Theme::font_heading(), 0);
    return btn;
}

lv_obj_t* create_numpad(lv_obj_t* parent, lv_obj_t* target_textarea, bool allow_decimal, bool allow_negative) {
    lv_obj_t* cont = create_card(parent);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    static int32_t cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t rows[] = {72, 72, 72, 72, 72, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, cols, rows);
    lv_obj_set_size(cont, 300, LV_SIZE_CONTENT);

    std::array<const char*, 12> keys{"7", "8", "9", "4", "5", "6", "1", "2", "3", allow_negative ? "-" : "C", "0", allow_decimal ? "." : "←"};
    for (size_t i = 0; i < keys.size(); ++i) {
        lv_obj_t* btn = create_gold_button(cont, keys[i]);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 3, 1, LV_GRID_ALIGN_STRETCH, i / 3, 1);
        lv_obj_add_event_cb(btn, keypad_event, LV_EVENT_CLICKED, target_textarea);
    }
    lv_obj_t* backspace = create_gold_button(cont, "←");
    lv_obj_set_grid_cell(backspace, LV_GRID_ALIGN_STRETCH, 0, 3, LV_GRID_ALIGN_STRETCH, 4, 1);
    lv_obj_add_event_cb(backspace, keypad_event, LV_EVENT_CLICKED, target_textarea);
    return cont;
}

} // namespace ui
