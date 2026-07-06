#include "apps/ShoppingListApp.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include "core/Storage.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
constexpr const char* PATH = "/shopping_list.txt";
constexpr const char* kCategories[] = {"All", "Produce", "Dairy", "Meat", "Pantry", "Other"};
constexpr int kCatCount = 6;
} // anonymous namespace

void ShoppingListApp::on_mount(lv_obj_t* parent) { root_ = parent; load(); build_ui(parent); rebuild_list(); }

void ShoppingListApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* outer = lv_obj_create(parent);
    lv_obj_remove_style_all(outer);
    lv_obj_set_size(outer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(outer, ui::Spacing::LG, 0);
    lv_obj_set_layout(outer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(outer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(outer, ui::Spacing::MD, 0);

    // Header: SHOPPING LIST + "+ ADD ITEM"
    ui::create_app_header(outer, "SHOPPING LIST", "+ ADD ITEM",
        [](lv_event_t* e){ static_cast<ShoppingListApp*>(lv_event_get_user_data(e))->show_add_dialog(); },
        this);

    // Category filter tabs
    lv_obj_t* tabs_row = lv_obj_create(outer);
    lv_obj_remove_style_all(tabs_row);
    lv_obj_set_size(tabs_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(tabs_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tabs_row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(tabs_row, ui::Spacing::SM, 0);

    for (int i = 0; i < kCatCount; ++i) {
        filter_btns_[i] = ui::create_gold_button(tabs_row, kCategories[i]);
        lv_obj_set_user_data(filter_btns_[i], reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(filter_btns_[i], [](lv_event_t* e){
            auto* app = static_cast<ShoppingListApp*>(lv_event_get_user_data(e));
            app->active_filter_ = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
            app->rebuild_list();
        }, LV_EVENT_CLICKED, this);
    }

    // Scrollable 2-column item list
    list_ = lv_obj_create(outer);
    lv_obj_remove_style_all(list_);
    lv_obj_set_size(list_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(list_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(list_, ui::Spacing::SM, 0);
    lv_obj_set_scroll_dir(list_, LV_DIR_VER);
}

void ShoppingListApp::show_add_dialog() {
    lv_obj_t* overlay = lv_obj_create(root_);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
    lv_obj_move_foreground(overlay);

    lv_obj_t* dialog = ui::create_card(overlay);
    lv_obj_set_size(dialog, 460, LV_SIZE_CONTENT);
    lv_obj_center(dialog);
    lv_obj_set_style_border_color(dialog, lv_color_hex(ui::Color::GOLD), 0);
    lv_obj_set_style_border_width(dialog, 1, 0);
    lv_obj_set_layout(dialog, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dialog, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(dialog, ui::Spacing::SM, 0);

    lv_obj_t* hdr = lv_obj_create(dialog);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* t = ui::create_section_title(hdr, "Add Item"); (void)t;
    lv_obj_t* close = ui::create_gold_button(hdr, LV_SYMBOL_CLOSE);
    lv_obj_add_event_cb(close, [](lv_event_t* e){
        // close → hdr → dialog → overlay  (3 levels)
        lv_obj_delete(lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent(lv_event_get_target_obj(e)))));
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* ta = lv_textarea_create(dialog);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "Item name...");
    lv_obj_set_style_bg_color(ta, lv_color_hex(ui::Color::SURFACE_2), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(ui::Color::GOLD_DIM), 0);
    // Tap to open keyboard
    lv_obj_add_event_cb(ta, [](lv_event_t* e){
        ui::show_styled_keyboard(lv_obj_get_screen(lv_event_get_target_obj(e)), lv_event_get_target_obj(e));
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* cat_lbl = lv_label_create(dialog);
    lv_label_set_text(cat_lbl, "Category");
    lv_obj_set_style_text_font(cat_lbl, ui::Theme::font_small(), 0);
    lv_obj_set_style_text_color(cat_lbl, lv_color_hex(ui::Color::TEXT_HINT), 0);

    lv_obj_t* cat_dd = lv_dropdown_create(dialog);
    lv_dropdown_set_options(cat_dd, "Produce\nDairy\nMeat\nPantry\nOther");
    lv_obj_set_width(cat_dd, LV_PCT(100));
    lv_obj_set_style_bg_color(cat_dd, lv_color_hex(ui::Color::SURFACE_2), 0);
    lv_obj_set_style_border_color(cat_dd, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_text_color(cat_dd, lv_color_hex(ui::Color::TEXT_PRI), 0);

    lv_obj_t* add_btn = ui::create_gold_button(dialog, "Add to List");
    lv_obj_set_user_data(dialog, ta); // store ta reference
    lv_obj_add_event_cb(add_btn, [](lv_event_t* e){
        auto* app = static_cast<ShoppingListApp*>(lv_event_get_user_data(e));
        lv_obj_t* dlg = lv_obj_get_parent(lv_event_get_target_obj(e));
        lv_obj_t* text_ta = static_cast<lv_obj_t*>(lv_obj_get_user_data(dlg));
        // Dialog children in order: 0=header, 1=textarea, 2=category-label, 3=dropdown, 4=add-btn
        lv_obj_t* dd = lv_obj_get_child(dlg, 3);
        const char* txt = lv_textarea_get_text(text_ta);
        if (*txt) {
            char cat[24]; lv_dropdown_get_selected_str(dd, cat, sizeof(cat));
            app->items_.push_back({txt, cat, false});
            app->save();
            app->rebuild_list();
        }
        lv_obj_delete(lv_obj_get_parent(dlg)); // close overlay
    }, LV_EVENT_CLICKED, this);
}

void ShoppingListApp::load() {
    items_.clear();
    std::string data;
    if (!core::Storage::instance().read_file(PATH, data)) return;
    std::istringstream ss(data);
    std::string line;
    while (std::getline(ss, line)) {
        std::istringstream ls(line);
        std::string done, category, text;
        if (std::getline(ls, done, '|') && std::getline(ls, category, '|') && std::getline(ls, text))
            items_.push_back({text, category, done == "1"});
    }
}

void ShoppingListApp::save() {
    std::ostringstream ss;
    for (const auto& item : items_) ss << (item.done ? '1' : '0') << '|' << item.category << '|' << item.text << '\n';
    core::Storage::instance().write_file(PATH, ss.str());
}

void ShoppingListApp::rebuild_list() {
    if (!list_) return;
    lv_obj_clean(list_);

    // Update tab labels with counts
    if (filter_btns_[0]) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "All (%d)", static_cast<int>(items_.size()));
        lv_obj_t* lbl = lv_obj_get_child(filter_btns_[0], 0);
        if (lbl) lv_label_set_text(lbl, buf);
        // Highlight active tab
        for (int i = 0; i < kCatCount; ++i) {
            if (filter_btns_[i] == nullptr) continue;
            lv_obj_set_style_bg_color(filter_btns_[i],
                (i == active_filter_) ? lv_color_hex(ui::Color::GOLD_DIM) : lv_color_hex(ui::Color::GOLD_FAINT), 0);
        }
    }

    for (size_t i = 0; i < items_.size(); ++i) {
        auto& item = items_[i];
        // Filter by category (active_filter_ 0 = All)
        if (active_filter_ > 0) {
            const char* cat = kCategories[active_filter_];
            if (item.category != cat) continue;
        }

        // Each item: ~50% width card with checkbox
        lv_obj_t* card = ui::create_card(list_);
        lv_obj_set_width(card, LV_PCT(49));
        lv_obj_set_height(card, LV_SIZE_CONTENT);
        lv_obj_set_layout(card, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(card, ui::Spacing::SM, 0);

        lv_obj_t* cb = lv_checkbox_create(card);
        lv_checkbox_set_text(cb, item.text.c_str());
        if (item.done) lv_obj_add_state(cb, LV_STATE_CHECKED);
        lv_obj_set_style_text_color(cb, item.done ? lv_color_hex(ui::Color::TEXT_HINT) : lv_color_hex(ui::Color::TEXT_PRI), 0);
        lv_obj_set_user_data(cb, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(cb, [](lv_event_t* e){
            auto* app = static_cast<ShoppingListApp*>(lv_event_get_user_data(e));
            const size_t idx = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
            if (idx < app->items_.size()) {
                app->items_[idx].done = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED);
                app->save();
                app->rebuild_list();
            }
        }, LV_EVENT_VALUE_CHANGED, this);

        // Long-press to delete
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(card, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(card, [](lv_event_t* e){
            auto* app = static_cast<ShoppingListApp*>(lv_event_get_user_data(e));
            const size_t idx = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
            if (idx < app->items_.size()) {
                app->items_.erase(app->items_.begin() + idx);
                app->save(); app->rebuild_list();
            }
        }, LV_EVENT_LONG_PRESSED, this);
    }
}

void ShoppingListApp::on_unmount() { root_ = nullptr; list_ = nullptr; for (auto& b : filter_btns_) b = nullptr; }
void ShoppingListApp::on_update(float) {}

} // namespace apps

