#include "apps/TimersApp.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include "core/Notifications.h"
#include "core/PowerManager.h"
#include "core/Settings.h"
#include "services/AudioService.h"
#include "ui/Animations.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
constexpr std::array<uint32_t, KitchenTimer::SLOT_COUNT> kColors{{ui::Color::TIMER_1, ui::Color::TIMER_2, ui::Color::TIMER_3, ui::Color::TIMER_4, ui::Color::TIMER_5}};
constexpr std::array<uint32_t, 5> kPresets{{300, 600, 900, 1800, 3600}};
constexpr std::array<const char*, 5> kPresetLabels{{"5m", "10m", "15m", "30m", "1h"}};

void slot_click(lv_event_t* e) {
    auto* app = static_cast<TimersApp*>(lv_event_get_user_data(e));
    const int slot = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    auto& timer = app->timers_[slot];
    if (!timer.active) {
        app->show_add_timer_dialog(app->root_);
        app->active_slot_ = slot;
        return;
    }
    // Dismiss an expired timer (clear it) on tap
    if (timer.expired) {
        services::AudioService::instance().stop_alarm();
        app->timers_[slot] = KitchenTimer{};
        app->timers_[slot].color = kColors[slot];
        app->update_timer_display(slot);
        const bool any_expired = std::any_of(app->timers_.begin(), app->timers_.end(),
            [](const auto& t){ return t.expired; });
        if (!any_expired) {
            core::PowerManager::instance().set_screensaver_blocked(false);
        }
        return;
    }
    timer.running = !timer.running;
    if (timer.remaining_sec == 0) timer.remaining_sec = timer.duration_sec;
    app->update_timer_display(slot);
    core::PowerManager::instance().reset_activity();
}

void slot_long_press(lv_event_t* e) {
    auto* app = static_cast<TimersApp*>(lv_event_get_user_data(e));
    const int slot = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    app->timers_[slot] = KitchenTimer{};
    app->timers_[slot].color = kColors[slot];
    app->update_timer_display(slot);
    // Re-evaluate whether screensaver should be unblocked
    const bool any_expired = std::any_of(app->timers_.begin(), app->timers_.end(),
        [](const auto& t){ return t.expired; });
    if (!any_expired) {
        services::AudioService::instance().stop_alarm();
        core::PowerManager::instance().set_screensaver_blocked(false);
    }
}

std::string format_time(uint32_t seconds) {
    char buf[24];
    const uint32_t h = seconds / 3600;
    const uint32_t m = (seconds % 3600) / 60;
    const uint32_t s = seconds % 60;
    if (h > 0) std::snprintf(buf, sizeof(buf), "%u:%02u:%02u", static_cast<unsigned>(h), static_cast<unsigned>(m), static_cast<unsigned>(s));
    else std::snprintf(buf, sizeof(buf), "%02u:%02u", static_cast<unsigned>(m), static_cast<unsigned>(s));
    return buf;
}
} // anonymous namespace

void TimersApp::on_mount(lv_obj_t* parent) {
    for (size_t i = 0; i < timers_.size(); ++i) timers_[i].color = kColors[i];
    root_ = parent;
    build_ui(parent);
}

void TimersApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* col = lv_obj_create(parent);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(col, ui::Spacing::LG, 0);
    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, ui::Spacing::MD, 0);

    ui::create_app_header(col, "TIMERS", "+ NEW TIMER",
        [](lv_event_t* e){ static_cast<TimersApp*>(lv_event_get_user_data(e))->show_add_timer_dialog(static_cast<TimersApp*>(lv_event_get_user_data(e))->root_); },
        this);

    lv_obj_t* grid = lv_obj_create(col);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100));
    static int32_t cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t rows[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, cols, rows);
    lv_obj_set_style_pad_column(grid, ui::Spacing::MD, 0);

    for (int i = 0; i < KitchenTimer::SLOT_COUNT; ++i) {
        timer_cards_[i] = ui::create_card(grid);
        lv_obj_set_grid_cell(timer_cards_[i], LV_GRID_ALIGN_STRETCH, i, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
        lv_obj_set_user_data(timer_cards_[i], reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_flag(timer_cards_[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(timer_cards_[i], slot_click, LV_EVENT_CLICKED, this);
        lv_obj_add_event_cb(timer_cards_[i], slot_long_press, LV_EVENT_LONG_PRESSED, this);
        lv_obj_set_layout(timer_cards_[i], LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(timer_cards_[i], LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(timer_cards_[i], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(timer_cards_[i], ui::Spacing::SM, 0);

        name_labels_[i] = lv_label_create(timer_cards_[i]);
        lv_obj_set_style_text_color(name_labels_[i], lv_color_hex(ui::Color::TEXT_SEC), 0);
        lv_obj_set_style_text_font(name_labels_[i], ui::Theme::font_small(), 0);

        arcs_[i] = ui::create_progress_ring(timer_cards_[i], kColors[i], 140);

        time_labels_[i] = lv_label_create(timer_cards_[i]);
        ui::style_number_label(time_labels_[i], kColors[i]);

        play_btns_[i] = lv_button_create(timer_cards_[i]);
        lv_obj_set_size(play_btns_[i], 40, 40);
        lv_obj_set_style_radius(play_btns_[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(play_btns_[i], lv_color_hex(ui::Color::GOLD_FAINT), 0);
        lv_obj_set_style_border_color(play_btns_[i], lv_color_hex(ui::Color::GOLD_DIM), 0);
        lv_obj_set_style_border_width(play_btns_[i], 1, 0);
        lv_obj_set_user_data(play_btns_[i], reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(play_btns_[i], slot_click, LV_EVENT_CLICKED, this);
        lv_obj_t* play_icon = lv_label_create(play_btns_[i]);
        lv_label_set_text(play_icon, LV_SYMBOL_PAUSE);
        lv_obj_set_style_text_color(play_icon, lv_color_hex(ui::Color::GOLD), 0);
        lv_obj_center(play_icon);

        update_timer_display(i);
    }
}

void TimersApp::on_unmount() { root_ = nullptr; for (auto& p : play_btns_) p = nullptr; for (auto& c : timer_cards_) c = nullptr; for (auto& a : arcs_) a = nullptr; for (auto& t : time_labels_) t = nullptr; for (auto& n : name_labels_) n = nullptr; }

bool TimersApp::has_active_timers() const {
    return std::any_of(timers_.begin(), timers_.end(), [](const auto& t){ return t.active && t.running; });
}

void TimersApp::add_timer(uint32_t duration_sec, const char* name, const char* emoji) {
    for (int i = 0; i < KitchenTimer::SLOT_COUNT; ++i) {
        if (!timers_[i].active || timers_[i].expired) {
            auto& timer = timers_[i];
            timer.duration_sec = duration_sec;
            timer.remaining_sec = duration_sec;
            timer.active = true;
            timer.expired = false;
            timer.running = true;
            std::strncpy(timer.name, name, sizeof(timer.name) - 1);
            std::strncpy(timer.emoji, emoji, sizeof(timer.emoji) - 1);
            timer.emoji[sizeof(timer.emoji) - 1] = '\0';
            update_timer_display(i);
            return;
        }
    }
    core::Notifications::instance().push(core::NotificationType::WARNING, "Timers full", "All five timer slots are in use.");
}

void TimersApp::update_timer_display(int slot) {
    auto& timer = timers_[slot];
    if (!time_labels_[slot]) return;
    if (!timer.active) {
        lv_label_set_text(time_labels_[slot], "+");
        lv_label_set_text(name_labels_[slot], "Add timer");
        ui::anim::animate_arc_to(arcs_[slot], 0);
        ui::anim::stop_pulse(timer_cards_[slot]);
        ui::anim::stop_blink(time_labels_[slot]);
        lv_obj_set_style_arc_color(arcs_[slot], lv_color_hex(timer.color), LV_PART_INDICATOR);
        ui::style_number_label(time_labels_[slot], timer.color);
        if (play_btns_[slot]) lv_obj_add_flag(play_btns_[slot], LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (play_btns_[slot]) {
        lv_obj_remove_flag(play_btns_[slot], LV_OBJ_FLAG_HIDDEN);
        lv_obj_t* play_icon = lv_obj_get_child(play_btns_[slot], 0);
        if (play_icon) lv_label_set_text(play_icon, timer.running ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }

    const int value = timer.duration_sec == 0 ? 0
        : static_cast<int>((timer.duration_sec - timer.remaining_sec) * 100 / timer.duration_sec);
    ui::anim::animate_arc_to(arcs_[slot], value);

    lv_label_set_text(time_labels_[slot], timer.expired ? "Done" : format_time(timer.remaining_sec).c_str());
    lv_label_set_text(name_labels_[slot], timer.name);

    if (timer.expired) {
        ui::anim::pulse_glow(timer_cards_[slot], ui::Color::GOLD_HI, 500);
        ui::anim::blink(time_labels_[slot], 350);
        lv_obj_set_style_arc_color(arcs_[slot], lv_color_hex(ui::Color::GOLD_HI), LV_PART_INDICATOR);
        ui::style_number_label(time_labels_[slot], ui::Color::GOLD_HI);
    } else if (timer.remaining_sec <= 30) {
        ui::anim::stop_pulse(timer_cards_[slot]);
        ui::anim::blink(time_labels_[slot], 350);
        lv_obj_set_style_arc_color(arcs_[slot], lv_color_hex(ui::Color::ERROR), LV_PART_INDICATOR);
        ui::style_number_label(time_labels_[slot], ui::Color::ERROR);
    } else if (timer.remaining_sec <= 60) {
        ui::anim::stop_pulse(timer_cards_[slot]);
        ui::anim::stop_blink(time_labels_[slot]);
        lv_obj_set_style_arc_color(arcs_[slot], lv_color_hex(ui::Color::WARNING), LV_PART_INDICATOR);
        ui::style_number_label(time_labels_[slot], ui::Color::WARNING);
    } else {
        ui::anim::stop_pulse(timer_cards_[slot]);
        ui::anim::stop_blink(time_labels_[slot]);
        lv_obj_set_style_arc_color(arcs_[slot], lv_color_hex(timer.color), LV_PART_INDICATOR);
        ui::style_number_label(time_labels_[slot], timer.color);
    }
}

void TimersApp::handle_timer_expired(int slot) {
    auto& timer = timers_[slot];
    timer.running = false;
    timer.expired = true;
    core::Notifications::instance().push(core::NotificationType::ALARM, "Timer finished", timer.name);

    const auto& s = core::Settings::instance().get();
    if (s.alarm_type != 3) {
        services::AudioService::instance().play_alarm(s.alarm_type, s.alarm_volume);
    }

    // Block screensaver while alarm is ringing so it never appears mid-alert
    core::PowerManager::instance().set_screensaver_blocked(true);
    core::PowerManager::instance().reset_activity();

    update_timer_display(slot);
}

void TimersApp::show_add_timer_dialog(lv_obj_t* parent) {
    lv_obj_t* overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
    lv_obj_move_foreground(overlay);

    // Dialog fills most of the screen height and is left-of-center (timers are on left side)
    lv_obj_t* dialog = ui::create_card(overlay);
    lv_obj_set_size(dialog, 460, lv_pct(94));
    lv_obj_align(dialog, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(ui::Color::GOLD_HI), 0);
    lv_obj_set_style_border_width(dialog, 1, 0);
    lv_obj_set_layout(dialog, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dialog, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(dialog, ui::Spacing::XS, 0);  // tighter rows
    lv_obj_add_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(dialog, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(dialog, LV_SCROLL_SNAP_NONE);

    // ── Header ──────────────────────────────────────────────────────────────
    lv_obj_t* hdr_row = lv_obj_create(dialog);
    lv_obj_remove_style_all(hdr_row);
    lv_obj_set_size(hdr_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(hdr_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ui::create_section_title(hdr_row, "New Timer");
    lv_obj_t* close_btn = ui::create_gold_button(hdr_row, LV_SYMBOL_CLOSE);
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e){
        // overlay is grandparent of dialog, and parent of overlay is app root
        lv_obj_t* btn     = lv_event_get_target_obj(e);
        lv_obj_t* hdr     = lv_obj_get_parent(btn);
        lv_obj_t* dlg     = lv_obj_get_parent(hdr);
        lv_obj_t* ov      = lv_obj_get_parent(dlg);
        lv_obj_delete(ov);
    }, LV_EVENT_CLICKED, nullptr);

    // ── Name chips (no label text — saves vertical space) ───────────────────

    struct NameEntry { const char* label; const char* icon; };
    static constexpr std::array<NameEntry, 6> kNames{{
        {"Pasta",   LV_SYMBOL_BELL},
        {"Rice",    LV_SYMBOL_BELL},
        {"Eggs",    LV_SYMBOL_BELL},
        {"Cake",    LV_SYMBOL_BELL},
        {"Meat",    LV_SYMBOL_BELL},
        {"Custom",  LV_SYMBOL_EDIT},
    }};

    // We need a label that shows the chosen name — store in dialog user_data pair
    struct DialogCtx { lv_obj_t* minutes_ta; lv_obj_t* name_label_display; char chosen_name[32]; };
    auto* ctx = new DialogCtx{};
    std::strncpy(ctx->chosen_name, "Timer", sizeof(ctx->chosen_name) - 1);
    lv_obj_set_user_data(dialog, ctx);
    lv_obj_add_event_cb(dialog, [](lv_event_t* e){
        delete static_cast<DialogCtx*>(lv_obj_get_user_data(lv_event_get_target_obj(e)));
    }, LV_EVENT_DELETE, nullptr);

    lv_obj_t* name_chips = lv_obj_create(dialog);
    lv_obj_remove_style_all(name_chips);
    lv_obj_set_size(name_chips, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(name_chips, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(name_chips, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(name_chips, ui::Spacing::XS, 0);

    for (size_t i = 0; i < kNames.size(); ++i) {
        lv_obj_t* chip = lv_button_create(name_chips);
        lv_obj_set_style_radius(chip, 20, 0);
        lv_obj_set_style_pad_hor(chip, 12, 0);
        lv_obj_set_style_pad_ver(chip, 6, 0);
        const bool first = (i == 0);
        lv_obj_set_style_bg_color(chip, lv_color_hex(first ? ui::Color::GOLD_DIM : ui::Color::SURFACE_2), 0);
        lv_obj_set_style_border_color(chip, lv_color_hex(ui::Color::GOLD_DIM), 0);
        lv_obj_set_style_border_width(chip, 1, 0);
        lv_obj_t* chip_lbl = lv_label_create(chip);
        lv_label_set_text(chip_lbl, kNames[i].label);
        lv_obj_set_style_text_font(chip_lbl, ui::Theme::font_small(), 0);
        lv_obj_set_style_text_color(chip_lbl, lv_color_hex(first ? ui::Color::GOLD_HI : ui::Color::TEXT_SEC), 0);
        // Store name string pointer in chip
        lv_obj_set_user_data(chip, const_cast<char*>(kNames[i].label));
        lv_obj_add_event_cb(chip, [](lv_event_t* e) {
            lv_obj_t* clicked = lv_event_get_target_obj(e);
            const char* name  = static_cast<const char*>(lv_obj_get_user_data(clicked));
            lv_obj_t* row     = lv_obj_get_parent(clicked);
            lv_obj_t* dlg     = lv_obj_get_parent(row);
            auto* ctx_        = static_cast<DialogCtx*>(lv_obj_get_user_data(dlg));
            if (ctx_) std::strncpy(ctx_->chosen_name, name, sizeof(ctx_->chosen_name) - 1);
            // Highlight selected chip
            const uint32_t cnt = lv_obj_get_child_count(row);
            for (uint32_t j = 0; j < cnt; ++j) {
                lv_obj_t* sib = lv_obj_get_child(row, j);
                const bool sel = (sib == clicked);
                lv_obj_set_style_bg_color(sib, lv_color_hex(sel ? ui::Color::GOLD_DIM : ui::Color::SURFACE_2), 0);
                lv_obj_t* l = lv_obj_get_child(sib, 0);
                if (l) lv_obj_set_style_text_color(l, lv_color_hex(sel ? ui::Color::GOLD_HI : ui::Color::TEXT_SEC), 0);
            }
        }, LV_EVENT_CLICKED, nullptr);
    }
    // Pre-select first chip
    std::strncpy(ctx->chosen_name, kNames[0].label, sizeof(ctx->chosen_name) - 1);

    // ── Time presets (compact labels, single row) ────────────────────────────

    lv_obj_t* presets_row = lv_obj_create(dialog);
    lv_obj_remove_style_all(presets_row);
    lv_obj_set_size(presets_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(presets_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(presets_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(presets_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(presets_row, ui::Spacing::XS, 0);

    for (size_t i = 0; i < kPresets.size(); ++i) {
        lv_obj_t* btn = ui::create_gold_button(presets_row, kPresetLabels[i]);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<uintptr_t>(kPresets[i])));
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            auto* app_    = static_cast<TimersApp*>(lv_event_get_user_data(e));
            lv_obj_t* btn_ = lv_event_get_target_obj(e);
            const auto secs = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn_)));
            lv_obj_t* row_  = lv_obj_get_parent(btn_);
            lv_obj_t* dlg_  = lv_obj_get_parent(row_);
            auto* ctx_      = static_cast<DialogCtx*>(lv_obj_get_user_data(dlg_));
            const char* name_ = ctx_ ? ctx_->chosen_name : "Timer";
            app_->add_timer(secs, name_, LV_SYMBOL_BELL);
            lv_obj_delete(lv_obj_get_parent(dlg_));  // delete overlay
        }, LV_EVENT_CLICKED, this);
    }

    // ── Custom minutes (no label text) ──────────────────────────────────────

    lv_obj_t* ta = lv_textarea_create(dialog);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "15");
    lv_obj_set_style_text_font(ta, ui::Theme::font_title(), 0);
    lv_obj_set_style_bg_color(ta, lv_color_hex(ui::Color::SURFACE_2), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(ui::Color::GOLD_DIM), 0);
    ctx->minutes_ta = ta;

    ui::create_numpad(dialog, ta, false, false);

    lv_obj_t* add_btn = ui::create_gold_button(dialog, LV_SYMBOL_PLAY "  Start Timer");
    lv_obj_set_width(add_btn, LV_PCT(100));
    lv_obj_add_event_cb(add_btn, [](lv_event_t* e) {
        auto* app_  = static_cast<TimersApp*>(lv_event_get_user_data(e));
        lv_obj_t* btn_  = lv_event_get_target_obj(e);
        lv_obj_t* dlg_  = lv_obj_get_parent(btn_);
        auto* ctx_  = static_cast<DialogCtx*>(lv_obj_get_user_data(dlg_));
        if (!ctx_) return;
        const int minutes = std::atoi(lv_textarea_get_text(ctx_->minutes_ta));
        if (minutes > 0) {
            app_->add_timer(static_cast<uint32_t>(minutes * 60), ctx_->chosen_name, LV_SYMBOL_BELL);
        }
        lv_obj_delete(lv_obj_get_parent(dlg_));  // delete overlay
    }, LV_EVENT_CLICKED, this);
}

void TimersApp::on_background_tick(float delta_sec) {
    // Tick timers while app is not visible — no UI calls
    second_accumulator_ += delta_sec;
    if (second_accumulator_ < 1.0f) return;
    second_accumulator_ -= 1.0f;
    for (int i = 0; i < KitchenTimer::SLOT_COUNT; ++i) {
        auto& timer = timers_[i];
        if (timer.active && timer.running && !timer.expired) {
            if (timer.remaining_sec > 0) timer.remaining_sec--;
            if (timer.remaining_sec == 0) handle_timer_expired(i);
        }
    }
}

void TimersApp::on_update(float delta_sec) {
    const float prev_acc = second_accumulator_;
    on_background_tick(delta_sec);
    // If a second ticked, refresh displays (update_timer_display guards against null labels)
    if (second_accumulator_ < prev_acc || second_accumulator_ == 0.0f) {
        for (int i = 0; i < KitchenTimer::SLOT_COUNT; ++i) {
            if (timers_[i].active || i == 0) update_timer_display(i);
        }
    }
}

} // namespace apps
