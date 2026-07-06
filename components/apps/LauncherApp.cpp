#include "apps/LauncherApp.h"

#include <array>
#include <cstdio>
#include "core/AppManager.h"
#include "core/Navigation.h"
#include "core/Network.h"
#include "core/PowerManager.h"
#include "services/TimeService.h"
#include "ui/Animations.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
struct AppEntry { core::AppId id; const char* name; const char* icon; };
constexpr std::array<AppEntry, 12> kApps{{
    {core::AppId::TIMERS,             "Timers",     "⏱"},
    {core::AppId::CONVERTER,          "Converter",  "⇄"},
    {core::AppId::INGREDIENT_SCALER,  "Scaler",     "⚖"},
    {core::AppId::BREAD_HYDRATION,    "Hydration",  "🍞"},
    {core::AppId::DRINK_CHILLER,      "Chiller",    "🧊"},
    {core::AppId::MEAT_TEMPERATURES,  "Meat Temps", "🥩"},
    {core::AppId::DISHWASHER,         "Dishwasher", "🫧"},
    {core::AppId::WEATHER,            "Weather",    "☀"},
    {core::AppId::RECIPES,            "Recipes",    "📖"},
    {core::AppId::LIGHTING,           "Lighting",   "💡"},
    {core::AppId::SHOPPING_LIST,      "Shopping",   "🛒"},
    {core::AppId::NOTES,              "Notes",      "✎"},
}};

// Quick-launch items shown on the home screen
constexpr std::array<AppEntry, 5> kQuick{{
    {core::AppId::TIMERS,    "Timers",    "⏱"},
    {core::AppId::RECIPES,   "Recipes",   "📖"},
    {core::AppId::CONVERTER, "Converter", "⇄"},
    {core::AppId::WEATHER,   "Weather",   "☀"},
    {core::AppId::COUNT,     "More",      "⋯"},  // COUNT = sentinel → show grid
}};

void launch_app(lv_event_t* e) {
    auto* entry = static_cast<const AppEntry*>(lv_event_get_user_data(e));
    core::PowerManager::instance().reset_activity();
    if (entry->id == core::AppId::COUNT) {
        // "More" — show grid overlay; handled separately
        return;
    }
    core::Navigation::instance().navigate_to(entry->id, core::AppManager::Transition::SLIDE_LEFT);
}

const char* greeting_prefix() {
    const int hour = services::TimeService::instance().hour();
    if (hour < 12) return "Good morning,";
    if (hour < 17) return "Good afternoon,";
    return "Good evening,";
}
} // anonymous namespace

void LauncherApp::on_mount(lv_obj_t* parent) {
    root_ = parent;
    show_home(parent);
}

void LauncherApp::show_home(lv_obj_t* parent) {
    lv_obj_clean(parent);
    lv_obj_set_style_bg_color(parent, lv_color_hex(ui::Color::BG), 0);

    lv_obj_t* main = lv_obj_create(parent);
    lv_obj_remove_style_all(main);
    lv_obj_set_size(main, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(main, ui::Spacing::LG, 0);
    lv_obj_set_layout(main, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(main, ui::Spacing::MD, 0);

    // ── Header: HOME / clock / status ────────────────────────────────────────
    lv_obj_t* hdr = lv_obj_create(main);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, LV_PCT(100), 40);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* home_lbl = lv_label_create(hdr);
    lv_label_set_text(home_lbl, "HOME");
    lv_obj_set_style_text_font(home_lbl, ui::Theme::font_heading(), 0);
    lv_obj_set_style_text_color(home_lbl, lv_color_hex(ui::Color::GOLD_HI), 0);

    lv_obj_t* status_row = lv_obj_create(hdr);
    lv_obj_remove_style_all(status_row);
    lv_obj_set_layout(status_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(status_row, ui::Spacing::MD, 0);
    clock_label_ = lv_label_create(status_row);
    lv_obj_set_style_text_font(clock_label_, ui::Theme::font_body(), 0);
    lv_obj_set_style_text_color(clock_label_, lv_color_hex(ui::Color::TEXT_SEC), 0);
    wifi_label_ = lv_label_create(status_row);
    lv_obj_set_style_text_color(wifi_label_, lv_color_hex(ui::Color::GOLD), 0);
    battery_label_ = lv_label_create(status_row);

    // ── Greeting ─────────────────────────────────────────────────────────────
    lv_obj_t* greeting_row = lv_obj_create(main);
    lv_obj_remove_style_all(greeting_row);
    lv_obj_set_size(greeting_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(greeting_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(greeting_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(greeting_row, 2, 0);

    greeting_label_ = lv_label_create(greeting_row);
    lv_obj_set_style_text_font(greeting_label_, ui::Theme::font_body(), 0);
    lv_obj_set_style_text_color(greeting_label_, lv_color_hex(ui::Color::TEXT_SEC), 0);

    lv_obj_t* chef_lbl = lv_label_create(greeting_row);
    lv_label_set_text(chef_lbl, "Chef.");
    lv_obj_set_style_text_font(chef_lbl, ui::Theme::font_title(), 0);
    lv_obj_set_style_text_color(chef_lbl, lv_color_hex(ui::Color::GOLD_HI), 0);

    lv_obj_t* sub_lbl = lv_label_create(greeting_row);
    lv_label_set_text(sub_lbl, "Ready to create something amazing?");
    lv_obj_set_style_text_font(sub_lbl, ui::Theme::font_label(), 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(ui::Color::TEXT_HINT), 0);

    // ── Hero card ─────────────────────────────────────────────────────────────
    lv_obj_t* hero = ui::create_card(main);
    lv_obj_set_size(hero, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(hero, 160, 0);
    lv_obj_set_layout(hero, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_color(hero, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_bg_color(hero, lv_color_hex(ui::Color::SURFACE_2), 0);

    lv_obj_t* hero_text = lv_obj_create(hero);
    lv_obj_remove_style_all(hero_text);
    lv_obj_set_flex_grow(hero_text, 1);
    lv_obj_set_layout(hero_text, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hero_text, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(hero_text, ui::Spacing::SM, 0);
    date_label_ = lv_label_create(hero_text);
    lv_obj_set_style_text_color(date_label_, lv_color_hex(ui::Color::TEXT_SEC), 0);
    lv_obj_set_style_text_font(date_label_, ui::Theme::font_label(), 0);
    lv_obj_t* tagline = lv_label_create(hero_text);
    lv_label_set_text(tagline, "Your kitchen assistant,\nalways ready.");
    lv_obj_set_style_text_font(tagline, ui::Theme::font_body(), 0);
    lv_obj_set_style_text_color(tagline, lv_color_hex(ui::Color::TEXT_PRI), 0);
    lv_label_set_long_mode(tagline, LV_LABEL_LONG_WRAP);

    lv_obj_t* hero_icon = lv_label_create(hero);
    lv_label_set_text(hero_icon, "🍽");
    lv_obj_set_style_text_font(hero_icon, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(hero_icon, lv_color_hex(ui::Color::GOLD), 0);

    // ── Quick-launch row ──────────────────────────────────────────────────────
    lv_obj_t* quick_row = lv_obj_create(main);
    lv_obj_remove_style_all(quick_row);
    lv_obj_set_size(quick_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(quick_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(quick_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(quick_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(quick_row, ui::Spacing::SM, 0);

    for (size_t i = 0; i < kQuick.size(); ++i) {
        lv_obj_t* card = ui::create_card(quick_row);
        lv_obj_set_flex_grow(card, 1);
        lv_obj_set_style_min_height(card, 88, 0);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_layout(card, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(card, ui::Spacing::XS, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(ui::Color::GOLD_HI), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(card, 2, LV_STATE_PRESSED);

        lv_obj_t* icon = lv_label_create(card);
        lv_label_set_text(icon, kQuick[i].icon);
        lv_obj_set_style_text_font(icon, ui::Theme::font_heading(), 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(ui::Color::GOLD_HI), 0);

        lv_obj_t* name = lv_label_create(card);
        lv_label_set_text(name, kQuick[i].name);
        lv_obj_set_style_text_font(name, ui::Theme::font_small(), 0);
        lv_obj_set_style_text_color(name, lv_color_hex(ui::Color::TEXT_SEC), 0);

        if (kQuick[i].id == core::AppId::COUNT) {
            // "More" button → show full grid
            lv_obj_add_event_cb(card, [](lv_event_t* ev) {
                auto* app = static_cast<LauncherApp*>(lv_event_get_user_data(ev));
                app->show_all_apps(lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent(lv_event_get_target_obj(ev))))));
            }, LV_EVENT_CLICKED, this);
        } else {
            lv_obj_add_event_cb(card, launch_app, LV_EVENT_CLICKED, const_cast<AppEntry*>(&kQuick[i]));
        }
        ui::anim::fade_in(card, 280, static_cast<uint32_t>(i) * 50);
    }

    on_update(0.0f);
}

void LauncherApp::show_all_apps(lv_obj_t* parent) {
    if (parent == nullptr) parent = root_;
    lv_obj_clean(parent);
    lv_obj_set_style_bg_color(parent, lv_color_hex(ui::Color::BG), 0);

    lv_obj_t* main = lv_obj_create(parent);
    lv_obj_remove_style_all(main);
    lv_obj_set_size(main, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(main, ui::Spacing::LG, 0);
    lv_obj_set_layout(main, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(main, ui::Spacing::MD, 0);

    // Header with back button
    lv_obj_t* hdr = lv_obj_create(main);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, LV_PCT(100), 40);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* title_lbl = lv_label_create(hdr);
    lv_label_set_text(title_lbl, "ALL APPS");
    lv_obj_set_style_text_font(title_lbl, ui::Theme::font_heading(), 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(ui::Color::GOLD_HI), 0);

    lv_obj_t* back_btn = ui::create_gold_button(hdr, LV_SYMBOL_LEFT " Home");
    lv_obj_add_event_cb(back_btn, [](lv_event_t* ev) {
        auto* app = static_cast<LauncherApp*>(lv_event_get_user_data(ev));
        app->show_home(app->root_);
    }, LV_EVENT_CLICKED, this);

    // 3×4 app grid
    lv_obj_t* grid = lv_obj_create(main);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100));
    static int32_t cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t rows[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, cols, rows);
    lv_obj_set_style_pad_row(grid, ui::Spacing::MD, 0);
    lv_obj_set_style_pad_column(grid, ui::Spacing::MD, 0);

    for (size_t i = 0; i < kApps.size(); ++i) {
        lv_obj_t* card = ui::create_card(grid);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, i % 3, 1, LV_GRID_ALIGN_STRETCH, i / 3, 1);
        lv_obj_set_layout(card, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_color(card, lv_color_hex(ui::Color::GOLD_HI), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(card, 2, LV_STATE_PRESSED);
        lv_obj_add_event_cb(card, launch_app, LV_EVENT_CLICKED, const_cast<AppEntry*>(&kApps[i]));

        lv_obj_t* icon = lv_label_create(card);
        lv_label_set_text(icon, kApps[i].icon);
        lv_obj_set_style_text_font(icon, ui::Theme::font_heading(), 0);

        lv_obj_t* name = lv_label_create(card);
        lv_label_set_text(name, kApps[i].name);
        lv_obj_set_style_text_font(name, ui::Theme::font_label(), 0);
        lv_obj_set_style_text_color(name, lv_color_hex(ui::Color::GOLD), 0);

        ui::anim::fade_in(card, 280, static_cast<uint32_t>(i) * 40);
    }
}

void LauncherApp::on_unmount() { root_ = nullptr; clock_label_ = nullptr; date_label_ = nullptr; wifi_label_ = nullptr; battery_label_ = nullptr; greeting_label_ = nullptr; }

void LauncherApp::on_update(float) {
    if (root_ == nullptr || clock_label_ == nullptr) return;

    lv_label_set_text(clock_label_, services::TimeService::instance().time_string(false).c_str());
    lv_label_set_text(date_label_, services::TimeService::instance().date_string().c_str());
    lv_label_set_text(greeting_label_, greeting_prefix());

    const auto& pm = core::PowerManager::instance();
    const uint8_t pct = pm.battery_percent();
    const auto charge = pm.charge_state();
    char battery[40];
    if (charge == core::ChargeState::CHARGING) {
        std::snprintf(battery, sizeof(battery), "⚡ %u%%", static_cast<unsigned>(pct));
        lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::SUCCESS), 0);
    } else if (charge == core::ChargeState::FULL) {
        std::snprintf(battery, sizeof(battery), "⚡ Full");
        lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::SUCCESS), 0);
    } else if (pct <= 15) {
        std::snprintf(battery, sizeof(battery), "🪫 %u%%", static_cast<unsigned>(pct));
        lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::ERROR), 0);
    } else {
        std::snprintf(battery, sizeof(battery), "🔋 %u%%", static_cast<unsigned>(pct));
        lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::GOLD), 0);
    }
    lv_label_set_text(battery_label_, battery);

    const bool connected = core::Network::instance().is_connected();
    lv_label_set_text(wifi_label_, connected ? LV_SYMBOL_WIFI " " : "");
}

} // namespace apps
