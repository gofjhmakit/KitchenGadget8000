#include "apps/RecipesApp.h"

#include <algorithm>
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
} // anonymous namespace

void RecipesApp::on_mount(lv_obj_t* parent) {
    root_ = parent;
    load_recipes();
    build_ui(parent);
    show_list();
}

void RecipesApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* outer = lv_obj_create(parent);
    lv_obj_remove_style_all(outer);
    lv_obj_set_size(outer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(outer, ui::Spacing::LG, 0);
    lv_obj_set_layout(outer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(outer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(outer, ui::Spacing::MD, 0);

    header_ = lv_obj_create(outer);
    lv_obj_remove_style_all(header_);
    lv_obj_set_size(header_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(header_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    header_title_ = lv_label_create(header_);
    lv_label_set_text(header_title_, "RECIPES");
    lv_obj_set_style_text_font(header_title_, ui::Theme::font_title(), 0);
    lv_obj_set_style_text_color(header_title_, lv_color_hex(ui::Color::GOLD_HI), 0);

    back_btn_ = ui::create_gold_button(header_, "< Back");
    lv_obj_add_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(back_btn_, [](lv_event_t* e){
        static_cast<RecipesApp*>(lv_event_get_user_data(e))->show_list();
    }, LV_EVENT_CLICKED, this);

    content_ = lv_obj_create(outer);
    lv_obj_remove_style_all(content_);
    lv_obj_set_size(content_, LV_PCT(100), LV_PCT(100));
}

void RecipesApp::load_recipes() {
    recipes_.clear();
    const auto files = core::Storage::instance().list_files("/recipes", ".md");
    for (const auto& file : files) {
        services::Recipe recipe;
        if (services::MarkdownParser::instance().parse_file(
                core::Storage::instance().base_path() + "/recipes/" + file, recipe)) {
            recipes_.push_back(recipe);
        }
    }
}

void RecipesApp::show_list() {
    step_mode_ = true;
    step_index_ = 0;
    if (!content_) return;

    lv_obj_add_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(header_title_, "RECIPES");

    lv_obj_clean(content_);
    lv_obj_set_layout(content_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(content_, ui::Spacing::MD, 0);
    lv_obj_set_scroll_dir(content_, LV_DIR_VER);

    if (recipes_.empty()) {
        lv_obj_t* lbl = lv_label_create(content_);
        lv_label_set_text(lbl, "No recipes found.\nAdd .md files to /spiffs/recipes/");
        lv_obj_set_style_text_color(lbl, lv_color_hex(ui::Color::TEXT_HINT), 0);
        lv_obj_center(lbl);
        return;
    }

    for (size_t i = 0; i < recipes_.size(); ++i) {
        const auto& r = recipes_[i];

        lv_obj_t* card = ui::create_card(content_);
        lv_obj_set_size(card, 300, LV_SIZE_CONTENT);
        lv_obj_set_layout(card, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(card, ui::Spacing::SM, 0);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(card, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(card, [](lv_event_t* e){
            auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e));
            const size_t idx = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
            app->show_detail(idx);
        }, LV_EVENT_CLICKED, this);

        // Recipe image or emoji placeholder
        ui::create_recipe_image(card, r.image_path, r.image.empty() ? "🍽" : r.image.c_str(), 300, 160);

        lv_obj_t* title_lbl = lv_label_create(card);
        lv_label_set_text(title_lbl, r.title.c_str());
        lv_obj_set_style_text_font(title_lbl, ui::Theme::font_body_bold(), 0);
        lv_obj_set_style_text_color(title_lbl, lv_color_hex(ui::Color::GOLD_HI), 0);
        lv_obj_set_width(title_lbl, LV_PCT(100));
        lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_WRAP);

        char meta[80];
        std::snprintf(meta, sizeof(meta), "%s  •  %d servings", r.time.c_str(), r.servings);
        lv_obj_t* meta_lbl = lv_label_create(card);
        lv_label_set_text(meta_lbl, meta);
        lv_obj_set_style_text_font(meta_lbl, ui::Theme::font_small(), 0);
        lv_obj_set_style_text_color(meta_lbl, lv_color_hex(ui::Color::TEXT_SEC), 0);
    }
}

void RecipesApp::show_detail(size_t index) {
    if (index >= recipes_.size()) return;
    current_index_ = index;
    const auto& r = recipes_[index];

    lv_obj_remove_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(header_title_, r.title.c_str());

    lv_obj_clean(content_);
    lv_obj_set_layout(content_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(content_, ui::Spacing::LG, 0);
    lv_obj_set_scroll_dir(content_, LV_DIR_NONE);

    // Left: image + ingredients
    lv_obj_t* left = lv_obj_create(content_);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, 340, LV_PCT(100));
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(left, ui::Spacing::SM, 0);
    lv_obj_set_scroll_dir(left, LV_DIR_VER);

    ui::create_recipe_image(left, r.image_path, r.image.empty() ? "🍽" : r.image.c_str(), 340, 200);

    lv_obj_t* ing_title = ui::create_section_title(left, "INGREDIENTS");
    (void)ing_title;
    for (const auto& ing : r.ingredients) {
        lv_obj_t* cb = lv_checkbox_create(left);
        lv_checkbox_set_text(cb, ing.c_str());
        lv_obj_set_style_text_color(cb, lv_color_hex(ui::Color::TEXT_PRI), 0);
    }

    // Right: step view
    lv_obj_t* right = lv_obj_create(content_);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, ui::Spacing::SM, 0);
    lv_obj_set_scroll_dir(right, LV_DIR_VER);

    const size_t total_steps = r.instructions.size();

    // Step counter header
    char step_hdr[48];
    if (total_steps > 0)
        std::snprintf(step_hdr, sizeof(step_hdr), "STEP %u OF %u", static_cast<unsigned>(step_index_ + 1), static_cast<unsigned>(total_steps));
    else
        std::snprintf(step_hdr, sizeof(step_hdr), "INSTRUCTIONS");

    lv_obj_t* step_lbl = lv_label_create(right);
    lv_label_set_text(step_lbl, step_hdr);
    lv_obj_set_style_text_font(step_lbl, ui::Theme::font_small(), 0);
    lv_obj_set_style_text_color(step_lbl, lv_color_hex(ui::Color::GOLD_DIM), 0);

    // Step instruction
    if (step_index_ < total_steps) {
        lv_obj_t* inst_lbl = lv_label_create(right);
        lv_obj_set_width(inst_lbl, LV_PCT(100));
        lv_label_set_long_mode(inst_lbl, LV_LABEL_LONG_WRAP);
        lv_label_set_text(inst_lbl, r.instructions[step_index_].c_str());
        lv_obj_set_style_text_font(inst_lbl, ui::Theme::font_body(), 0);
        lv_obj_set_style_text_color(inst_lbl, lv_color_hex(ui::Color::TEXT_PRI), 0);

        // Timer buttons extracted from this step
        const auto timers = services::MarkdownParser::instance().extract_timer_seconds(r.instructions[step_index_]);
        for (uint32_t secs : timers) {
            char btn_txt[64];
            std::snprintf(btn_txt, sizeof(btn_txt), "⏱ Start %u min timer", secs / 60);
            lv_obj_t* btn = ui::create_gold_button(right, btn_txt);
            lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<uintptr_t>(secs)));
            lv_obj_add_event_cb(btn, timer_from_text, LV_EVENT_CLICKED, this);
        }
    } else if (!r.notes.empty()) {
        lv_obj_t* notes_lbl = lv_label_create(right);
        lv_obj_set_width(notes_lbl, LV_PCT(100));
        lv_label_set_long_mode(notes_lbl, LV_LABEL_LONG_WRAP);
        lv_label_set_text(notes_lbl, r.notes.c_str());
        lv_obj_set_style_text_color(notes_lbl, lv_color_hex(ui::Color::TEXT_SEC), 0);
    }

    // Dot progress indicators
    if (total_steps > 1) {
        lv_obj_t* dots_row = lv_obj_create(right);
        lv_obj_remove_style_all(dots_row);
        lv_obj_set_size(dots_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(dots_row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(dots_row, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_gap(dots_row, 6, 0);
        for (size_t di = 0; di < std::min(total_steps, static_cast<size_t>(12)); ++di) {
            lv_obj_t* dot = lv_obj_create(dots_row);
            lv_obj_remove_style_all(dot);
            lv_obj_set_size(dot, 10, 10);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(dot, di == step_index_ ? lv_color_hex(ui::Color::GOLD_HI) : lv_color_hex(ui::Color::GOLD_FAINT), 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        }
    }

    // PREV / NEXT navigation at bottom
    lv_obj_t* nav_row = lv_obj_create(right);
    lv_obj_remove_style_all(nav_row);
    lv_obj_set_size(nav_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(nav_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(nav_row, ui::Spacing::MD, 0);

    lv_obj_t* prev_btn = ui::create_gold_button(nav_row, "← PREV");
    if (step_index_ == 0) lv_obj_add_state(prev_btn, LV_STATE_DISABLED);
    lv_obj_add_event_cb(prev_btn, [](lv_event_t* e){
        auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e));
        if (app->step_index_ > 0) { app->step_index_--; app->show_detail(app->current_index_); }
    }, LV_EVENT_CLICKED, this);

    lv_obj_t* next_btn = ui::create_gold_button(nav_row, "NEXT →");
    if (step_index_ + 1 >= total_steps) {
        if (step_index_ + 1 == total_steps && !r.notes.empty()) {
            lv_label_set_text(lv_obj_get_child(next_btn, 0), "NOTES →");
        } else {
            lv_obj_add_state(next_btn, LV_STATE_DISABLED);
        }
    }
    lv_obj_add_event_cb(next_btn, [](lv_event_t* e){
        auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e));
        const size_t total = app->recipes_[app->current_index_].instructions.size();
        const bool has_notes = !app->recipes_[app->current_index_].notes.empty();
        if (app->step_index_ < total + (has_notes ? 0 : 0)) {
            app->step_index_++;
            app->show_detail(app->current_index_);
        }
    }, LV_EVENT_CLICKED, this);
}

void RecipesApp::on_unmount() { root_ = nullptr; content_ = nullptr; header_ = nullptr; back_btn_ = nullptr; header_title_ = nullptr; }
void RecipesApp::on_update(float) {}

} // namespace apps

