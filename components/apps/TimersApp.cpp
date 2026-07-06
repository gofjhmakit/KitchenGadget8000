#include "apps/TimersApp.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include "core/Notifications.h"
#include "core/PowerManager.h"
#include "ui/Animations.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
constexpr std::array<uint32_t, KitchenTimer::SLOT_COUNT> kColors{{ui::Color::TIMER_1, ui::Color::TIMER_2, ui::Color::TIMER_3, ui::Color::TIMER_4, ui::Color::TIMER_5}};
constexpr std::array<uint32_t, 5> kPresets{{300, 600, 900, 1800, 3600}};
constexpr std::array<const char*, 5> kPresetLabels{{"5 min", "10 min", "15 min", "30 min", "1 hr"}};

void slot_click(lv_event_t* e) {
    auto* app = static_cast<TimersApp*>(lv_event_get_user_data(e));
    const int slot = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    auto& timer = app->timers_[slot];
    if (!timer.active) {
        app->show_add_timer_dialog(app->root_);
        app->active_slot_ = slot;
        return;
    }
    timer.running = !timer.running;
    if (!timer.expired && timer.remaining_sec == 0) {
        timer.remaining_sec = timer.duration_sec;
    }
    core::PowerManager::instance().reset_activity();
}

void slot_long_press(lv_event_t* e) {
    auto* app = static_cast<TimersApp*>(lv_event_get_user_data(e));
    const int slot = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    app->timers_[slot] = KitchenTimer{};
    app->timers_[slot].color = kColors[slot];
    app->update_timer_display(slot);
}

void preset_click(lv_event_t* e) {
    auto* app = static_cast<TimersApp*>(lv_event_get_user_data(e));
    const auto seconds = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
    app->add_timer(seconds, "Preset", "⏱");
}

std::string format_time(uint32_t seconds) {
    char buf[24];
    const uint32_t h = seconds / 3600;
    const uint32_t m = (seconds % 3600) / 60;
    const uint32_t s = seconds % 60;
    if (h > 0) std::snprintf(buf, sizeof(buf), "%u:%02u:%02u", h, m, s);
    else std::snprintf(buf, sizeof(buf), "%02u:%02u", m, s);
    return buf;
}

void dialog_add(lv_event_t* e) {
    auto* app = static_cast<TimersApp*>(lv_event_get_user_data(e));
    lv_obj_t* dialog = lv_obj_get_parent(lv_event_get_target_obj(e));
    lv_obj_t* ta = static_cast<lv_obj_t*>(lv_obj_get_user_data(dialog));
    const int minutes = std::atoi(lv_textarea_get_text(ta));
    if (minutes > 0) {
        app->add_timer(static_cast<uint32_t>(minutes * 60), "Custom", "⏱");
    }
    lv_obj_delete(dialog);
}
}

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
    lv_obj_set_style_pad_row(col, ui::Spacing::LG, 0);

    lv_obj_t* title = ui::create_section_title(col, "Kitchen Timers");
    (void)title;

    lv_obj_t* presets = lv_obj_create(col);
    lv_obj_remove_style_all(presets);
    lv_obj_set_width(presets, LV_PCT(100));
    lv_obj_set_layout(presets, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(presets, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(presets, ui::Spacing::SM, 0);
    for (size_t i = 0; i < kPresets.size(); ++i) {
        lv_obj_t* btn = ui::create_gold_button(presets, kPresetLabels[i]);
        lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<uintptr_t>(kPresets[i])));
        lv_obj_add_event_cb(btn, preset_click, LV_EVENT_CLICKED, this);
    }
    lv_obj_t* custom = ui::create_gold_button(presets, "Custom");
    lv_obj_add_event_cb(custom, [](lv_event_t* e){ static_cast<TimersApp*>(lv_event_get_user_data(e))->show_add_timer_dialog(static_cast<TimersApp*>(lv_event_get_user_data(e))->root_); }, LV_EVENT_CLICKED, this);

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

        arcs_[i] = ui::create_progress_ring(timer_cards_[i], kColors[i], 140);
        time_labels_[i] = lv_label_create(timer_cards_[i]);
        name_labels_[i] = lv_label_create(timer_cards_[i]);
        ui::style_number_label(time_labels_[i], kColors[i]);
        lv_obj_set_style_text_color(name_labels_[i], lv_color_hex(ui::Color::TEXT_SEC), 0);
        update_timer_display(i);
    }
}

void TimersApp::on_unmount() { root_ = nullptr; }

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
            std::snprintf(timer.name, sizeof(timer.name), "%s %s", emoji, name);
            std::strncpy(timer.emoji, emoji, sizeof(timer.emoji) - 1);
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
        lv_arc_set_value(arcs_[slot], 0);
        ui::anim::stop_pulse(timer_cards_[slot]);
        return;
    }
    const int value = timer.duration_sec == 0 ? 0 : static_cast<int>((timer.duration_sec - timer.remaining_sec) * 100 / timer.duration_sec);
    lv_arc_set_value(arcs_[slot], value);
    lv_label_set_text(time_labels_[slot], timer.expired ? "Done" : format_time(timer.remaining_sec).c_str());
    lv_label_set_text(name_labels_[slot], timer.name);
    if (timer.expired) ui::anim::pulse_glow(timer_cards_[slot], ui::Color::GOLD_HI, 600);
    else ui::anim::stop_pulse(timer_cards_[slot]);
}

void TimersApp::handle_timer_expired(int slot) {
    auto& timer = timers_[slot];
    timer.running = false;
    timer.expired = true;
    core::Notifications::instance().push(core::NotificationType::ALARM, "Timer finished", timer.name);
    update_timer_display(slot);
}

void TimersApp::show_add_timer_dialog(lv_obj_t* parent) {
    lv_obj_t* dialog = ui::create_card(parent);
    lv_obj_set_size(dialog, 420, 480);
    lv_obj_center(dialog);
    lv_obj_set_style_border_color(dialog, lv_color_hex(ui::Color::GOLD_HI), 0);
    lv_obj_set_layout(dialog, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dialog, LV_FLEX_FLOW_COLUMN);
    lv_obj_t* label = ui::create_section_title(dialog, "Minutes");
    (void)label;
    lv_obj_t* ta = lv_textarea_create(dialog);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "15");
    lv_obj_set_style_text_font(ta, ui::Theme::font_title(), 0);
    lv_obj_set_user_data(dialog, ta);
    ui::create_numpad(dialog, ta, false, false);
    lv_obj_t* add = ui::create_gold_button(dialog, "Add timer");
    lv_obj_add_event_cb(add, dialog_add, LV_EVENT_CLICKED, this);
}

void TimersApp::on_update(float delta_sec) {
    second_accumulator_ += delta_sec;
    if (second_accumulator_ < 1.0f) {
        return;
    }
    second_accumulator_ -= 1.0f;
    for (int i = 0; i < KitchenTimer::SLOT_COUNT; ++i) {
        auto& timer = timers_[i];
        if (timer.active && timer.running && !timer.expired) {
            if (timer.remaining_sec > 0) {
                const uint32_t before = timer.remaining_sec;
                timer.remaining_sec = before > 0 ? before - 1 : 0;
                if (before != timer.remaining_sec) update_timer_display(i);
            }
            if (timer.remaining_sec == 0) {
                handle_timer_expired(i);
            }
        }
    }
}

} // namespace apps
