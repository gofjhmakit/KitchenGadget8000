#include "apps/RecipesApp.h"

#include <algorithm>
#include <cstdio>
#include "apps/TimersApp.h"
#include "core/AppManager.h"
#include "core/BottomNav.h"
#include "core/Navigation.h"
#include "core/Storage.h"
#include "core/Notifications.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {

void timer_from_text(lv_event_t* e) {
    const uint32_t seconds = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
        lv_obj_get_user_data(lv_event_get_target_obj(e))));
    if (auto* base = core::AppManager::instance().app(core::AppId::TIMERS)) {
        static_cast<TimersApp*>(base)->add_timer(seconds, "Recipe", "🍳");
        core::Notifications::instance().push(core::NotificationType::SUCCESS, "Timer added", "Recipe timer created.");
    }
    core::Navigation::instance().navigate_to(core::AppId::TIMERS, core::AppManager::Transition::SLIDE_LEFT);
}

/// Build step content (counter, instruction, timers, dots, prev/next) into `col`.
/// Used by both portrait (single column) and landscape (right column).
void build_step_section(lv_obj_t* col, RecipesApp* app, const services::Recipe& r) {
    const size_t total_steps = r.instructions.size();
    const size_t step_idx   = app->step_index_;

    // Step counter
    char step_hdr[48];
    if (total_steps > 0)
        std::snprintf(step_hdr, sizeof(step_hdr), "STEP %u OF %u",
                      static_cast<unsigned>(step_idx + 1), static_cast<unsigned>(total_steps));
    else
        std::snprintf(step_hdr, sizeof(step_hdr), "INSTRUCTIONS");

    lv_obj_t* step_lbl = lv_label_create(col);
    lv_label_set_text(step_lbl, step_hdr);
    lv_obj_set_style_text_font(step_lbl, ui::Theme::font_small(), 0);
    lv_obj_set_style_text_color(step_lbl, lv_color_hex(ui::Color::GOLD_DIM), 0);

    if (step_idx < total_steps) {
        lv_obj_t* inst = lv_label_create(col);
        lv_obj_set_width(inst, LV_PCT(100));
        lv_label_set_long_mode(inst, LV_LABEL_LONG_WRAP);
        lv_label_set_text(inst, r.instructions[step_idx].c_str());
        lv_obj_set_style_text_font(inst, ui::Theme::font_body(), 0);
        lv_obj_set_style_text_color(inst, lv_color_hex(ui::Color::TEXT_PRI), 0);

        for (uint32_t secs : services::MarkdownParser::instance().extract_timer_seconds(r.instructions[step_idx])) {
            char btn_txt[64];
            std::snprintf(btn_txt, sizeof(btn_txt), LV_SYMBOL_BELL " Start %u min timer",
                          static_cast<unsigned>(secs / 60));
            lv_obj_t* btn = ui::create_gold_button(col, btn_txt);
            lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<uintptr_t>(secs)));
            lv_obj_add_event_cb(btn, timer_from_text, LV_EVENT_CLICKED, app);
        }
    } else if (!r.notes.empty()) {
        lv_obj_t* notes = lv_label_create(col);
        lv_obj_set_width(notes, LV_PCT(100));
        lv_label_set_long_mode(notes, LV_LABEL_LONG_WRAP);
        lv_label_set_text(notes, r.notes.c_str());
        lv_obj_set_style_text_color(notes, lv_color_hex(ui::Color::TEXT_SEC), 0);
    }

    // Dot indicators (max 16 shown)
    if (total_steps > 1) {
        lv_obj_t* dots = lv_obj_create(col);
        lv_obj_remove_style_all(dots);
        lv_obj_set_size(dots, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(dots, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_gap(dots, 6, 0);
        for (size_t di = 0; di < std::min(total_steps, static_cast<size_t>(16)); ++di) {
            lv_obj_t* dot = lv_obj_create(dots);
            lv_obj_remove_style_all(dot);
            lv_obj_set_size(dot, 10, 10);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            const uint32_t col_val = (di == step_idx) ? ui::Color::GOLD_HI : ui::Color::GOLD_FAINT;
            lv_obj_set_style_bg_color(dot, lv_color_hex(col_val), 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        }
    }

    // PREV / NEXT
    lv_obj_t* nav = lv_obj_create(col);
    lv_obj_remove_style_all(nav);
    lv_obj_set_size(nav, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(nav, ui::Spacing::MD, 0);

    lv_obj_t* prev_btn = ui::create_gold_button(nav, "← PREV");
    if (step_idx == 0) lv_obj_add_state(prev_btn, LV_STATE_DISABLED);
    lv_obj_add_event_cb(prev_btn, [](lv_event_t* e){
        auto* a = static_cast<RecipesApp*>(lv_event_get_user_data(e));
        if (a->step_index_ > 0) { a->step_index_--; a->show_detail(a->current_index_); }
    }, LV_EVENT_CLICKED, app);

    lv_obj_t* next_btn = ui::create_gold_button(nav, "NEXT →");
    const bool at_end     = (step_idx + 1 >= total_steps);
    const bool notes_next = (step_idx + 1 == total_steps && !r.notes.empty());
    if (notes_next) {
        lv_obj_t* child = lv_obj_get_child(next_btn, 0);
        if (child) lv_label_set_text(child, "NOTES →");
    } else if (at_end) {
        lv_obj_add_state(next_btn, LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(next_btn, [](lv_event_t* e){
        auto* a = static_cast<RecipesApp*>(lv_event_get_user_data(e));
        const size_t tot  = a->recipes_[a->current_index_].instructions.size();
        const bool   hnot = !a->recipes_[a->current_index_].notes.empty();
        const size_t max  = tot + (hnot ? 1 : 0);
        if (a->step_index_ + 1 < max) { a->step_index_++; a->show_detail(a->current_index_); }
    }, LV_EVENT_CLICKED, app);
}

} // anonymous namespace

// ── Lifecycle ────────────────────────────────────────────────────────────────

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

    // Persistent header row (hidden by default, shown in detail)
    header_ = lv_obj_create(outer);
    lv_obj_remove_style_all(header_);
    lv_obj_set_size(header_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(header_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    back_btn_ = ui::create_gold_button(header_, LV_SYMBOL_LEFT " Back");
    lv_obj_add_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(back_btn_, [](lv_event_t* e){
        static_cast<RecipesApp*>(lv_event_get_user_data(e))->show_list();
    }, LV_EVENT_CLICKED, this);

    header_title_ = lv_label_create(header_);
    lv_label_set_text(header_title_, "RECIPES");
    lv_obj_set_style_text_font(header_title_, ui::Theme::font_title(), 0);
    lv_obj_set_style_text_color(header_title_, lv_color_hex(ui::Color::GOLD_HI), 0);
    lv_obj_set_flex_grow(header_title_, 1);
    lv_obj_set_style_pad_left(header_title_, ui::Spacing::SM, 0);
    lv_label_set_long_mode(header_title_, LV_LABEL_LONG_DOT);

    // Action buttons (rotate + fullscreen) — rebuilt in show_detail()
    // placeholder so layout is stable; children replaced per detail render
    lv_obj_t* actions = lv_obj_create(header_);
    lv_obj_remove_style_all(actions);
    lv_obj_set_size(actions, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(actions, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(actions, ui::Spacing::SM, 0);
    lv_obj_add_flag(actions, LV_OBJ_FLAG_HIDDEN); // hidden in list view
    // store pointer in user_data of header_ for retrieval
    lv_obj_set_user_data(header_, actions);

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

// ── List view ────────────────────────────────────────────────────────────────

void RecipesApp::show_list() {
    // Restore landscape & nav when returning to list
    if (portrait_) {
        portrait_ = false;
        lv_display_set_rotation(lv_display_get_default(), LV_DISPLAY_ROTATION_0);
        core::BottomNav::instance().reposition();
    }
    if (fullscreen_) {
        fullscreen_ = false;
        core::BottomNav::instance().show();
    }
    if (root_) {
        const lv_coord_t dw = lv_display_get_horizontal_resolution(lv_display_get_default());
        const lv_coord_t dh = lv_display_get_vertical_resolution(lv_display_get_default());
        lv_obj_set_size(root_, dw, dh - core::BottomNav::HEIGHT);
    }

    step_index_ = 0;
    if (!content_) return;

    // Hide back/actions, show plain title
    lv_obj_add_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(header_title_, "RECIPES");
    lv_obj_set_style_text_font(header_title_, ui::Theme::font_title(), 0);
    lv_obj_set_flex_grow(header_title_, 0);
    if (header_) {
        auto* actions = static_cast<lv_obj_t*>(lv_obj_get_user_data(header_));
        if (actions) lv_obj_add_flag(actions, LV_OBJ_FLAG_HIDDEN);
    }

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
            const size_t idx = static_cast<size_t>(reinterpret_cast<uintptr_t>(
                lv_obj_get_user_data(lv_event_get_target_obj(e))));
            app->show_detail(idx);
        }, LV_EVENT_CLICKED, this);

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

// ── Detail view ──────────────────────────────────────────────────────────────

void RecipesApp::show_detail(size_t index) {
    if (index >= recipes_.size()) return;
    current_index_ = index;
    const auto& r   = recipes_[index];

    // Show back button + title
    lv_obj_remove_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(header_title_, r.title.c_str());
    lv_obj_set_style_text_font(header_title_, ui::Theme::font_label(), 0);
    lv_obj_set_flex_grow(header_title_, 1);

    // Rebuild action buttons (rotate + fullscreen)
    auto* actions = static_cast<lv_obj_t*>(lv_obj_get_user_data(header_));
    if (actions) {
        lv_obj_clean(actions);
        lv_obj_remove_flag(actions, LV_OBJ_FLAG_HIDDEN);

        // Rotate button: LV_SYMBOL_REFRESH shows current orientation
        lv_obj_t* rot_btn = ui::create_gold_button(actions,
            portrait_ ? LV_SYMBOL_REFRESH " Land" : LV_SYMBOL_REFRESH " Port");
        if (portrait_) {
            lv_obj_set_style_bg_color(rot_btn, lv_color_hex(ui::Color::GOLD_DIM), 0);
            lv_obj_set_style_bg_opa(rot_btn, LV_OPA_COVER, 0);
        }
        lv_obj_add_event_cb(rot_btn, [](lv_event_t* e){
            static_cast<RecipesApp*>(lv_event_get_user_data(e))->toggle_orientation();
        }, LV_EVENT_CLICKED, this);

        // Fullscreen toggle (hidden when portrait since portrait is always fullscreen)
        if (!portrait_) {
            lv_obj_t* fs_btn = ui::create_gold_button(actions,
                fullscreen_ ? LV_SYMBOL_MINUS " Exit" : LV_SYMBOL_PLUS " Full");
            lv_obj_add_event_cb(fs_btn, [](lv_event_t* e){
                auto* app = static_cast<RecipesApp*>(lv_event_get_user_data(e));
                if (app->fullscreen_) app->exit_fullscreen();
                else                  app->enter_fullscreen();
            }, LV_EVENT_CLICKED, this);
        }
    }

    // Rebuild content with orientation-aware layout
    lv_obj_clean(content_);
    lv_obj_set_scroll_dir(content_, LV_DIR_NONE);

    if (portrait_) {
        // ── Portrait: single scrollable column ──────────────────────────────
        lv_obj_set_layout(content_, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(content_, ui::Spacing::MD, 0);
        lv_obj_set_scroll_dir(content_, LV_DIR_VER);

        const lv_coord_t dw = lv_display_get_horizontal_resolution(lv_display_get_default());
        ui::create_recipe_image(content_, r.image_path,
            r.image.empty() ? "🍽" : r.image.c_str(), dw - 2 * ui::Spacing::LG, 220);

        // Step content (counter, text, timer buttons, dots, prev/next)
        build_step_section(content_, this, r);

        // Ingredients below steps
        lv_obj_t* sep = lv_obj_create(content_);
        lv_obj_remove_style_all(sep);
        lv_obj_set_size(sep, LV_PCT(100), 1);
        lv_obj_set_style_bg_color(sep, lv_color_hex(ui::Color::SEPARATOR), 0);
        lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);

        ui::create_section_title(content_, "INGREDIENTS");
        for (const auto& ing : r.ingredients) {
            lv_obj_t* cb = lv_checkbox_create(content_);
            lv_checkbox_set_text(cb, ing.c_str());
            lv_obj_set_style_text_color(cb, lv_color_hex(ui::Color::TEXT_PRI), 0);
        }

    } else {
        // ── Landscape: two-column layout ─────────────────────────────────────
        lv_obj_set_layout(content_, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(content_, ui::Spacing::LG, 0);

        // Left: image + ingredients (scrollable)
        lv_obj_t* left = lv_obj_create(content_);
        lv_obj_remove_style_all(left);
        lv_obj_set_size(left, 340, LV_PCT(100));
        lv_obj_set_layout(left, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(left, ui::Spacing::SM, 0);
        lv_obj_set_scroll_dir(left, LV_DIR_VER);

        // In fullscreen landscape, make image slightly taller
        const lv_coord_t img_h = fullscreen_ ? 230 : 200;
        ui::create_recipe_image(left, r.image_path,
            r.image.empty() ? "🍽" : r.image.c_str(), 340, img_h);

        ui::create_section_title(left, "INGREDIENTS");
        for (const auto& ing : r.ingredients) {
            lv_obj_t* cb = lv_checkbox_create(left);
            lv_checkbox_set_text(cb, ing.c_str());
            lv_obj_set_style_text_color(cb, lv_color_hex(ui::Color::TEXT_PRI), 0);
        }

        // Right: step content (scrollable)
        lv_obj_t* right = lv_obj_create(content_);
        lv_obj_remove_style_all(right);
        lv_obj_set_size(right, LV_PCT(100), LV_PCT(100));
        lv_obj_set_layout(right, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(right, ui::Spacing::SM, 0);
        lv_obj_set_scroll_dir(right, LV_DIR_VER);

        build_step_section(right, this, r);
    }
}

// ── Fullscreen ────────────────────────────────────────────────────────────────

void RecipesApp::enter_fullscreen() {
    if (fullscreen_ || portrait_) return; // portrait is already fullscreen
    fullscreen_ = true;
    core::BottomNav::instance().hide();
    const lv_coord_t dw = lv_display_get_horizontal_resolution(lv_display_get_default());
    const lv_coord_t dh = lv_display_get_vertical_resolution(lv_display_get_default());
    lv_obj_set_size(root_, dw, dh);
    show_detail(current_index_);
}

void RecipesApp::exit_fullscreen() {
    if (!fullscreen_ || portrait_) return; // can't exit if portrait (always fullscreen)
    fullscreen_ = false;
    auto& nav = core::BottomNav::instance();
    nav.show();
    const lv_coord_t dw = lv_display_get_horizontal_resolution(lv_display_get_default());
    const lv_coord_t dh = lv_display_get_vertical_resolution(lv_display_get_default());
    lv_obj_set_size(root_, dw, dh - core::BottomNav::HEIGHT);
    show_detail(current_index_);
}

// ── Orientation ───────────────────────────────────────────────────────────────

void RecipesApp::toggle_orientation() {
    portrait_ = !portrait_;
    lv_display_set_rotation(lv_display_get_default(),
        portrait_ ? LV_DISPLAY_ROTATION_90 : LV_DISPLAY_ROTATION_0);

    const lv_coord_t dw = lv_display_get_horizontal_resolution(lv_display_get_default());
    const lv_coord_t dh = lv_display_get_vertical_resolution(lv_display_get_default());
    auto& nav = core::BottomNav::instance();

    if (portrait_) {
        // Portrait always fullscreen — hide nav
        fullscreen_ = true;
        nav.hide();
        lv_obj_set_size(root_, dw, dh);
    } else {
        // Back to landscape — exit fullscreen, restore nav
        fullscreen_ = false;
        nav.reposition(); // recalc position for new display dims
        nav.show();
        lv_obj_set_size(root_, dw, dh - core::BottomNav::HEIGHT);
    }
    show_detail(current_index_);
}

void RecipesApp::on_unmount() {
    // Restore landscape + nav before leaving
    if (portrait_) {
        portrait_ = false;
        lv_display_set_rotation(lv_display_get_default(), LV_DISPLAY_ROTATION_0);
        core::BottomNav::instance().reposition();
    }
    if (fullscreen_) {
        fullscreen_ = false;
        core::BottomNav::instance().show();
    }
    root_         = nullptr;
    content_      = nullptr;
    header_       = nullptr;
    back_btn_     = nullptr;
    header_title_ = nullptr;
}

void RecipesApp::on_update(float) {}

} // namespace apps
