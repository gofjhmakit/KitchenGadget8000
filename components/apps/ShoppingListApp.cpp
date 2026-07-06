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
void rebuild(lv_event_t* e) { static_cast<ShoppingListApp*>(lv_event_get_user_data(e))->rebuild_list(); }
}

void ShoppingListApp::on_mount(lv_obj_t* parent) { root_ = parent; load(); build_ui(parent); rebuild_list(); }

void ShoppingListApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont, ui::Spacing::LG, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(cont, "Shopping list");
    lv_obj_t* controls = lv_obj_create(cont);
    lv_obj_remove_style_all(controls);
    lv_obj_set_layout(controls, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    item_ta_ = lv_textarea_create(controls); lv_obj_set_width(item_ta_, 340); lv_textarea_set_one_line(item_ta_, true); lv_textarea_set_placeholder_text(item_ta_, "Add item");
    category_dd_ = lv_dropdown_create(controls); lv_dropdown_set_options(category_dd_, "Produce\nDairy\nMeat\nPantry\nOther");
    lv_obj_t* add = ui::create_gold_button(controls, "Add");
    lv_obj_add_event_cb(add, [](lv_event_t* e){ auto* app = static_cast<ShoppingListApp*>(lv_event_get_user_data(e)); char cat[24]; lv_dropdown_get_selected_str(app->category_dd_, cat, sizeof(cat)); const char* txt = lv_textarea_get_text(app->item_ta_); if (*txt) { app->items_.push_back({txt, cat, false}); lv_textarea_set_text(app->item_ta_, ""); app->save(); app->rebuild_list(); } }, LV_EVENT_CLICKED, this);
    lv_obj_t* clear = ui::create_gold_button(controls, "Clear completed");
    lv_obj_add_event_cb(clear, [](lv_event_t* e){ auto* app = static_cast<ShoppingListApp*>(lv_event_get_user_data(e)); app->items_.erase(std::remove_if(app->items_.begin(), app->items_.end(), [](const auto& i){ return i.done; }), app->items_.end()); app->save(); app->rebuild_list(); }, LV_EVENT_CLICKED, this);
    list_ = lv_obj_create(cont);
    lv_obj_set_size(list_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(list_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_, LV_FLEX_FLOW_COLUMN);
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
        if (std::getline(ls, done, '|') && std::getline(ls, category, '|') && std::getline(ls, text)) items_.push_back({text, category, done == "1"});
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
    for (size_t i = 0; i < items_.size(); ++i) {
        auto& item = items_[i];
        char subtitle[64]; std::snprintf(subtitle, sizeof(subtitle), "%s", item.category.c_str());
        lv_obj_t* row = ui::create_card(list_);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_t* cb = lv_checkbox_create(row);
        lv_checkbox_set_text(cb, item.text.c_str());
        if (item.done) lv_obj_add_state(cb, LV_STATE_CHECKED);
        lv_obj_set_user_data(cb, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(cb, [](lv_event_t* e){ auto* app = static_cast<ShoppingListApp*>(lv_event_get_user_data(e)); const size_t idx = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e)))); if (idx < app->items_.size()) { app->items_[idx].done = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED); app->save(); } }, LV_EVENT_VALUE_CHANGED, this);
        lv_obj_t* cat = lv_label_create(row); lv_label_set_text(cat, subtitle); lv_obj_set_style_text_color(cat, lv_color_hex(ui::Color::TEXT_SEC), 0);
        lv_obj_t* del = ui::create_gold_button(row, "Delete");
        lv_obj_set_user_data(del, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(del, [](lv_event_t* e){ auto* app = static_cast<ShoppingListApp*>(lv_event_get_user_data(e)); const size_t idx = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e)))); if (idx < app->items_.size()) { app->items_.erase(app->items_.begin() + idx); app->save(); app->rebuild_list(); } }, LV_EVENT_CLICKED, this);
    }
}

void ShoppingListApp::on_unmount() { root_ = nullptr; }
void ShoppingListApp::on_update(float) {}

} // namespace apps
