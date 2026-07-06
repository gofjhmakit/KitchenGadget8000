#include "apps/RecipesApp.h"

#include <cstdio>
#include "apps/TimersApp.h"
#include "core/AppManager.h"
#include "core/Navigation.h"
#include "core/Storage.h"
#include "core/Notifications.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
void open_recipe(lv_event_t* e) {
    auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e));
    const size_t index = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    app->show_detail(index);
}

void back_to_list(lv_event_t* e) { static_cast<RecipesApp*>(lv_event_get_user_data(e))->show_list(); }
void toggle_step_mode(lv_event_t* e) {
    auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e));
    app->step_mode_ = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED);
    app->step_index_ = 0;
    app->show_detail(app->current_index_);
}

void timer_from_text(lv_event_t* e) {
    auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e));
    const uint32_t seconds = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    if (auto* base = core::AppManager::instance().app(core::AppId::TIMERS)) {
        if (auto* timers = dynamic_cast<TimersApp*>(base)) {
            timers->add_timer(seconds, "Recipe", "🍳");
            core::Notifications::instance().push(core::NotificationType::SUCCESS, "Timer added", "Recipe timer created.");
        }
    }
    core::Navigation::instance().navigate_to(core::AppId::TIMERS, core::AppManager::Transition::SLIDE_LEFT);
}
}

void RecipesApp::on_mount(lv_obj_t* parent) {
    root_ = parent;
    load_recipes();
    build_ui(parent);
    show_list();
}

void RecipesApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont, ui::Spacing::LG, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(cont, "Recipes");
    content_ = lv_obj_create(cont);
    lv_obj_remove_style_all(content_);
    lv_obj_set_size(content_, LV_PCT(100), LV_PCT(100));
}

void RecipesApp::load_recipes() {
    recipes_.clear();
    const auto files = core::Storage::instance().list_files("/recipes", ".md");
    for (const auto& file : files) {
        services::Recipe recipe;
        if (services::MarkdownParser::instance().parse_file(core::Storage::instance().base_path() + "/recipes/" + file, recipe)) {
            recipes_.push_back(recipe);
        }
    }
}

void RecipesApp::show_list() {
    if (!content_) return;
    lv_obj_clean(content_);
    lv_obj_set_layout(content_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(content_, LV_DIR_VER);
    for (size_t i = 0; i < recipes_.size(); ++i) {
        const auto& r = recipes_[i];
        char subtitle[160];
        std::snprintf(subtitle, sizeof(subtitle), "%s • %d servings", r.time.c_str(), r.servings);
        lv_obj_t* item = ui::create_list_item(content_, r.title.c_str(), subtitle, r.image.empty() ? "🍽" : r.image.c_str());
        lv_obj_set_user_data(item, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(item, open_recipe, LV_EVENT_CLICKED, this);
    }
}

void RecipesApp::show_detail(size_t index) {
    if (index >= recipes_.size()) return;
    current_index_ = index;
    lv_obj_clean(content_);
    lv_obj_set_layout(content_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(content_, LV_DIR_VER);
    const auto& r = recipes_[index];
    lv_obj_t* back = ui::create_gold_button(content_, "Back to recipes");
    lv_obj_add_event_cb(back, back_to_list, LV_EVENT_CLICKED, this);
    lv_obj_t* title = lv_label_create(content_);
    lv_label_set_text(title, r.title.c_str());
    lv_obj_set_style_text_font(title, ui::Theme::font_title(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(ui::Color::GOLD_HI), 0);
    char meta[192];
    std::snprintf(meta, sizeof(meta), "%s • %d servings • %s", r.time.c_str(), r.servings, r.image.c_str());
    lv_obj_t* meta_lbl = lv_label_create(content_);
    lv_label_set_text(meta_lbl, meta);

    lv_obj_t* toggle = lv_checkbox_create(content_);
    lv_checkbox_set_text(toggle, "One step at a time");
    if (step_mode_) lv_obj_add_state(toggle, LV_STATE_CHECKED);
    lv_obj_add_event_cb(toggle, toggle_step_mode, LV_EVENT_VALUE_CHANGED, this);

    ui::create_section_title(content_, "Ingredients");
    for (const auto& ingredient : r.ingredients) {
        lv_obj_t* cb = lv_checkbox_create(content_);
        lv_checkbox_set_text(cb, ingredient.c_str());
    }

    ui::create_section_title(content_, "Instructions");
    const size_t start = step_mode_ ? step_index_ : 0;
    const size_t end = step_mode_ ? std::min(step_index_ + 1, r.instructions.size()) : r.instructions.size();
    for (size_t i = start; i < end; ++i) {
        char line[512];
        std::snprintf(line, sizeof(line), "%u. %s", static_cast<unsigned>(i + 1), r.instructions[i].c_str());
        lv_obj_t* lbl = lv_label_create(content_);
        lv_obj_set_width(lbl, LV_PCT(100));
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_label_set_text(lbl, line);
        const auto timers = services::MarkdownParser::instance().extract_timer_seconds(r.instructions[i]);
        for (uint32_t seconds : timers) {
            char btn_txt[48];
            std::snprintf(btn_txt, sizeof(btn_txt), "Start %u min timer", seconds / 60);
            lv_obj_t* btn = ui::create_gold_button(content_, btn_txt);
            lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<uintptr_t>(seconds)));
            lv_obj_add_event_cb(btn, timer_from_text, LV_EVENT_CLICKED, this);
        }
    }
    if (step_mode_ && !r.instructions.empty()) {
        lv_obj_t* nav = lv_obj_create(content_);
        lv_obj_remove_style_all(nav);
        lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
        if (step_index_ > 0) {
            lv_obj_t* prev = ui::create_gold_button(nav, "Previous step");
            lv_obj_add_event_cb(prev, [](lv_event_t* e){ auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e)); if (app->step_index_ > 0) { app->step_index_--; app->show_detail(app->current_index_); } }, LV_EVENT_CLICKED, this);
        }
        if (step_index_ + 1 < r.instructions.size()) {
            lv_obj_t* next = ui::create_gold_button(nav, "Next step");
            lv_obj_add_event_cb(next, [](lv_event_t* e){ auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e)); app->step_index_++; app->show_detail(app->current_index_); }, LV_EVENT_CLICKED, this);
        }
    }
    if (!r.notes.empty()) {
        ui::create_section_title(content_, "Notes");
        lv_obj_t* notes = lv_label_create(content_);
        lv_obj_set_width(notes, LV_PCT(100));
        lv_label_set_long_mode(notes, LV_LABEL_LONG_WRAP);
        lv_label_set_text(notes, r.notes.c_str());
    }
}

void RecipesApp::on_unmount() { root_ = nullptr; }
void RecipesApp::on_update(float) {}

} // namespace apps
