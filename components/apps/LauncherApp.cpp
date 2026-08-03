#include "apps/LauncherApp.h"

#include <array>
#include <cstdio>
#include <cstring>
#include "core/AppManager.h"
#include "core/Navigation.h"
#include "core/Network.h"
#include "core/PowerManager.h"
#include "core/Settings.h"
#include "services/AudioService.h"
#include "services/TimeService.h"
#include "services/WiFiService.h"
#include "ui/Animations.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
struct AppEntry { core::AppId id; const char* name; const char* icon; };
constexpr std::array<AppEntry, 12> kApps{{
    {core::AppId::TIMERS,             "Timers",     LV_SYMBOL_BELL},
    {core::AppId::CONVERTER,          "Converter",  LV_SYMBOL_LOOP},
    {core::AppId::INGREDIENT_SCALER,  "Scaler",     LV_SYMBOL_SHUFFLE},
    {core::AppId::BREAD_HYDRATION,    "Hydration",  LV_SYMBOL_TINT},
    {core::AppId::DRINK_CHILLER,      "Chiller",    LV_SYMBOL_MINUS},
    {core::AppId::MEAT_TEMPERATURES,  "Meat Temps", LV_SYMBOL_WARNING},
    {core::AppId::DISHWASHER,         "Dishwasher", LV_SYMBOL_REFRESH},
    {core::AppId::WEATHER,            "Weather",    LV_SYMBOL_GPS},
    {core::AppId::RECIPES,            "Recipes",    LV_SYMBOL_LIST},
    {core::AppId::LIGHTING,           "Lighting",   LV_SYMBOL_CHARGE},
    {core::AppId::SHOPPING_LIST,      "Shopping",   LV_SYMBOL_SAVE},
    {core::AppId::NOTES,              "Notes",      LV_SYMBOL_EDIT},
}};

constexpr std::array<AppEntry, 5> kQuick{{
    {core::AppId::TIMERS,    "Timers",    LV_SYMBOL_BELL},
    {core::AppId::RECIPES,   "Recipes",   LV_SYMBOL_LIST},
    {core::AppId::CONVERTER, "Converter", LV_SYMBOL_LOOP},
    {core::AppId::WEATHER,   "Weather",   LV_SYMBOL_GPS},
    {core::AppId::COUNT,     "More",      LV_SYMBOL_BARS},
}};

struct WifiCtx {
    lv_obj_t* ssid_ta;
    lv_obj_t* pass_ta;
    lv_obj_t* overlay;
};

void launch_app(lv_event_t* e) {
    auto* entry = static_cast<const AppEntry*>(lv_event_get_user_data(e));
    core::PowerManager::instance().reset_activity();
    if (entry->id == core::AppId::COUNT) {
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

void destroy_wifi_ctx(lv_event_t* e) {
    auto* ctx = static_cast<WifiCtx*>(lv_event_get_user_data(e));
    delete ctx;
}
} // anonymous namespace

void LauncherApp::on_mount(lv_obj_t* parent) {
    root_ = parent;
    const int view = core::AppManager::instance().take_pending_view();
    if (view == 1) {
        show_all_apps(parent);
    } else {
        show_home(parent);
    }
}

void LauncherApp::show_home(lv_obj_t* parent) {
    if (parent == nullptr) {
        parent = root_;
    }
    if (parent == nullptr) {
        return;
    }
    root_ = parent;
    // Null labels before clean so on_update() won't write to stale pointers
    clock_label_ = nullptr; date_label_ = nullptr; wifi_label_ = nullptr;
    battery_label_ = nullptr; greeting_label_ = nullptr;
    lv_obj_clean(parent);
    lv_obj_set_style_bg_color(parent, lv_color_hex(ui::Color::BG), 0);

    lv_obj_t* main = lv_obj_create(parent);
    lv_obj_remove_style_all(main);
    lv_obj_set_size(main, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(main, ui::Spacing::LG, 0);
    lv_obj_set_layout(main, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(main, ui::Spacing::MD, 0);

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

    lv_obj_t* settings_btn = lv_button_create(status_row);
    lv_obj_remove_style_all(settings_btn);
    lv_obj_t* settings_icon = lv_label_create(settings_btn);
    lv_label_set_text(settings_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(settings_icon, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_text_color(settings_icon, lv_color_hex(ui::Color::GOLD_HI), LV_STATE_PRESSED);
    lv_obj_add_event_cb(settings_btn, [](lv_event_t* ev) {
        auto* app = static_cast<LauncherApp*>(lv_event_get_user_data(ev));
        app->show_settings_overlay();
    }, LV_EVENT_CLICKED, this);

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
    lv_label_set_text(hero_icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(hero_icon, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(hero_icon, lv_color_hex(ui::Color::GOLD), 0);

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
            lv_obj_add_event_cb(card, [](lv_event_t* ev) {
                auto* app = static_cast<LauncherApp*>(lv_event_get_user_data(ev));
                lv_async_call([](void* data) {
                    auto* a = static_cast<LauncherApp*>(data);
                    if (a != nullptr && a->root_ != nullptr) {
                        a->show_all_apps(a->root_);
                    }
                }, app);
            }, LV_EVENT_CLICKED, this);
        } else {
            lv_obj_add_event_cb(card, launch_app, LV_EVENT_CLICKED, const_cast<AppEntry*>(&kQuick[i]));
        }
        ui::anim::fade_in(card, 280, static_cast<uint32_t>(i) * 50);
    }

    on_update(0.0f);
}

void LauncherApp::show_all_apps(lv_obj_t* parent) {
    if (parent == nullptr) {
        parent = root_;
    }
    if (parent == nullptr) {
        return;
    }
    root_ = parent;
    // Null labels before clean so on_update() won't write to stale pointers
    clock_label_ = nullptr; date_label_ = nullptr; wifi_label_ = nullptr;
    battery_label_ = nullptr; greeting_label_ = nullptr;
    lv_obj_clean(parent);
    lv_obj_set_style_bg_color(parent, lv_color_hex(ui::Color::BG), 0);

    lv_obj_t* main = lv_obj_create(parent);
    lv_obj_remove_style_all(main);
    lv_obj_set_size(main, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(main, ui::Spacing::LG, 0);
    lv_obj_set_layout(main, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(main, ui::Spacing::MD, 0);

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
        lv_async_call([](void* data) {
            auto* a = static_cast<LauncherApp*>(data);
            if (a != nullptr && a->root_ != nullptr) {
                a->show_home(a->root_);
            }
        }, app);
    }, LV_EVENT_CLICKED, this);

    lv_obj_t* grid = lv_obj_create(main);
    lv_obj_remove_style_all(grid);
    lv_obj_set_flex_grow(grid, 1);  // take remaining height
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_add_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(grid, LV_SCROLL_SNAP_NONE);
    static int32_t cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    // 5 rows: 12 apps + 1 Settings card
    static int32_t rows[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
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

    // Settings card (13th, row 4 col 0)
    {
        lv_obj_t* card = ui::create_card(grid);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 4, 1);
        lv_obj_set_layout(card, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_color(card, lv_color_hex(ui::Color::GOLD_HI), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(card, 2, LV_STATE_PRESSED);
        lv_obj_set_style_border_color(card, lv_color_hex(ui::Color::GOLD_DIM), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_add_event_cb(card, [](lv_event_t* ev) {
            auto* app = static_cast<LauncherApp*>(lv_event_get_user_data(ev));
            app->show_settings_overlay();
        }, LV_EVENT_CLICKED, this);

        lv_obj_t* icon = lv_label_create(card);
        lv_label_set_text(icon, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_font(icon, ui::Theme::font_heading(), 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(ui::Color::GOLD_HI), 0);

        lv_obj_t* name = lv_label_create(card);
        lv_label_set_text(name, "Settings");
        lv_obj_set_style_text_font(name, ui::Theme::font_label(), 0);
        lv_obj_set_style_text_color(name, lv_color_hex(ui::Color::GOLD), 0);

        ui::anim::fade_in(card, 280, static_cast<uint32_t>(kApps.size()) * 40);
    }
}

void LauncherApp::show_settings_overlay() {
    lv_obj_t* overlay = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
    lv_obj_move_foreground(overlay);

    lv_obj_t* panel = ui::create_card(overlay);
    lv_obj_set_size(panel, 520, lv_pct(92));
    lv_obj_center(panel);
    lv_obj_set_style_border_color(panel, lv_color_hex(ui::Color::GOLD_HI), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(panel, ui::Spacing::MD, 0);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scroll_snap_y(panel, LV_SCROLL_SNAP_NONE);

    lv_obj_t* hdr = lv_obj_create(panel);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* title = lv_label_create(hdr);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_font(title, ui::Theme::font_heading(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(ui::Color::GOLD_HI), 0);
    lv_obj_t* close = ui::create_gold_button(hdr, LV_SYMBOL_CLOSE);
    lv_obj_add_event_cb(close, [](lv_event_t* e) {
        lv_obj_delete_async(static_cast<lv_obj_t*>(lv_event_get_user_data(e)));
    }, LV_EVENT_CLICKED, overlay);

    auto& cfg = core::Settings::instance().get();

    lv_obj_t* wifi_sec = lv_label_create(panel);
    lv_label_set_text(wifi_sec, LV_SYMBOL_WIFI "  WiFi");
    lv_obj_set_style_text_font(wifi_sec, ui::Theme::font_label(), 0);
    lv_obj_set_style_text_color(wifi_sec, lv_color_hex(ui::Color::GOLD), 0);

    lv_obj_t* ssid_row = lv_obj_create(panel);
    lv_obj_remove_style_all(ssid_row);
    lv_obj_set_size(ssid_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(ssid_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(ssid_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(ssid_row, ui::Spacing::SM, 0);
    lv_obj_t* ssid_lbl = lv_label_create(ssid_row);
    lv_label_set_text(ssid_lbl, "SSID");
    lv_obj_set_style_text_color(ssid_lbl, lv_color_hex(ui::Color::TEXT_SEC), 0);
    lv_obj_set_style_text_font(ssid_lbl, ui::Theme::font_small(), 0);
    lv_obj_set_width(ssid_lbl, 80);
    lv_obj_t* ssid_ta = lv_textarea_create(ssid_row);
    lv_obj_set_flex_grow(ssid_ta, 1);
    lv_textarea_set_one_line(ssid_ta, true);
    lv_textarea_set_text(ssid_ta, cfg.wifi_ssid);
    lv_obj_set_style_bg_color(ssid_ta, lv_color_hex(ui::Color::SURFACE_2), 0);
    lv_obj_set_style_border_color(ssid_ta, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_text_font(ssid_ta, ui::Theme::font_body(), 0);

    lv_obj_t* pass_row = lv_obj_create(panel);
    lv_obj_remove_style_all(pass_row);
    lv_obj_set_size(pass_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(pass_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pass_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(pass_row, ui::Spacing::SM, 0);
    lv_obj_t* pass_lbl = lv_label_create(pass_row);
    lv_label_set_text(pass_lbl, "Password");
    lv_obj_set_style_text_color(pass_lbl, lv_color_hex(ui::Color::TEXT_SEC), 0);
    lv_obj_set_style_text_font(pass_lbl, ui::Theme::font_small(), 0);
    lv_obj_set_width(pass_lbl, 80);
    lv_obj_t* pass_ta = lv_textarea_create(pass_row);
    lv_obj_set_flex_grow(pass_ta, 1);
    lv_textarea_set_one_line(pass_ta, true);
    lv_textarea_set_password_mode(pass_ta, true);
    lv_textarea_set_text(pass_ta, cfg.wifi_password);
    lv_obj_set_style_bg_color(pass_ta, lv_color_hex(ui::Color::SURFACE_2), 0);
    lv_obj_set_style_border_color(pass_ta, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_text_font(pass_ta, ui::Theme::font_body(), 0);

    lv_obj_t* kb = lv_keyboard_create(panel);
    lv_obj_set_size(kb, LV_PCT(100), 160);
    lv_obj_set_style_bg_color(kb, lv_color_hex(ui::Color::SURFACE), 0);
    lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, 0);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);  // hidden until textarea is focused
    lv_keyboard_set_textarea(kb, ssid_ta);
    lv_obj_add_event_cb(ssid_ta, [](lv_event_t* e) {
        lv_obj_t* kb_ = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
        lv_keyboard_set_textarea(kb_, lv_event_get_target_obj(e));
        lv_obj_remove_flag(kb_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_scroll_to_view(kb_, LV_ANIM_ON);
    }, LV_EVENT_FOCUSED, kb);
    lv_obj_add_event_cb(pass_ta, [](lv_event_t* e) {
        lv_obj_t* kb_ = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
        lv_keyboard_set_textarea(kb_, lv_event_get_target_obj(e));
        lv_obj_remove_flag(kb_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_scroll_to_view(kb_, LV_ANIM_ON);
    }, LV_EVENT_FOCUSED, kb);
    lv_obj_add_event_cb(kb, [](lv_event_t* e) {
        lv_obj_add_flag(lv_event_get_target_obj(e), LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(kb, [](lv_event_t* e) {
        lv_obj_add_flag(lv_event_get_target_obj(e), LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_CANCEL, nullptr);

    auto* wifi_ctx = new WifiCtx{ssid_ta, pass_ta, overlay};
    lv_obj_add_event_cb(overlay, destroy_wifi_ctx, LV_EVENT_DELETE, wifi_ctx);
    lv_obj_t* wifi_save = ui::create_gold_button(panel, LV_SYMBOL_WIFI "  Save WiFi & Connect");
    lv_obj_add_event_cb(wifi_save, [](lv_event_t* e) {
        auto* ctx = static_cast<WifiCtx*>(lv_event_get_user_data(e));
        auto& s = core::Settings::instance().get();
        std::strncpy(s.wifi_ssid, lv_textarea_get_text(ctx->ssid_ta), sizeof(s.wifi_ssid) - 1);
        s.wifi_ssid[sizeof(s.wifi_ssid) - 1] = '\0';
        std::strncpy(s.wifi_password, lv_textarea_get_text(ctx->pass_ta), sizeof(s.wifi_password) - 1);
        s.wifi_password[sizeof(s.wifi_password) - 1] = '\0';
        core::Settings::instance().save();
        services::WiFiService::instance().stop();
        services::WiFiService::instance().start(s.wifi_ssid, s.wifi_password);
        lv_obj_delete_async(ctx->overlay);
    }, LV_EVENT_CLICKED, wifi_ctx);

    lv_obj_t* alarm_sep = lv_label_create(panel);
    lv_label_set_text(alarm_sep, LV_SYMBOL_BELL "  Alarm");
    lv_obj_set_style_text_font(alarm_sep, ui::Theme::font_label(), 0);
    lv_obj_set_style_text_color(alarm_sep, lv_color_hex(ui::Color::GOLD), 0);

    lv_obj_t* type_row = lv_obj_create(panel);
    lv_obj_remove_style_all(type_row);
    lv_obj_set_size(type_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(type_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(type_row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(type_row, ui::Spacing::SM, 0);
    constexpr const char* kTypeLabels[] = {"Beep", "Chime", "Ding", "Mute"};
    for (int i = 0; i < 4; ++i) {
        lv_obj_t* btn = lv_button_create(type_row);
        lv_obj_set_style_radius(btn, 20, 0);
        lv_obj_set_style_pad_hor(btn, 16, 0);
        lv_obj_set_style_pad_ver(btn, 8, 0);
        const bool selected = (cfg.alarm_type == static_cast<uint8_t>(i));
        lv_obj_set_style_bg_color(btn, lv_color_hex(selected ? ui::Color::GOLD_DIM : ui::Color::SURFACE_2), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(ui::Color::GOLD_DIM), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, kTypeLabels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(selected ? ui::Color::GOLD_HI : ui::Color::TEXT_SEC), 0);
        lv_obj_set_style_text_font(lbl, ui::Theme::font_small(), 0);
        lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* clicked = lv_event_get_target_obj(e);
            const int idx = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(clicked)));
            core::Settings::instance().get().alarm_type = static_cast<uint8_t>(idx);
            core::Settings::instance().save();
            lv_obj_t* row = lv_obj_get_parent(clicked);
            const uint32_t cnt = lv_obj_get_child_count(row);
            for (uint32_t j = 0; j < cnt; ++j) {
                lv_obj_t* sib = lv_obj_get_child(row, j);
                const bool sel = (j == static_cast<uint32_t>(idx));
                lv_obj_set_style_bg_color(sib, lv_color_hex(sel ? ui::Color::GOLD_DIM : ui::Color::SURFACE_2), 0);
                lv_obj_t* l = lv_obj_get_child(sib, 0);
                if (l != nullptr) {
                    lv_obj_set_style_text_color(l, lv_color_hex(sel ? ui::Color::GOLD_HI : ui::Color::TEXT_SEC), 0);
                }
            }
            if (idx < 3) {
                services::AudioService::instance().play_alarm(static_cast<uint8_t>(idx), core::Settings::instance().get().alarm_volume);
            }
        }, LV_EVENT_CLICKED, nullptr);
    }

    lv_obj_t* vol_row = lv_obj_create(panel);
    lv_obj_remove_style_all(vol_row);
    lv_obj_set_size(vol_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(vol_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(vol_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(vol_row, ui::Spacing::SM, 0);
    lv_obj_t* vol_lbl = lv_label_create(vol_row);
    lv_label_set_text(vol_lbl, "Volume");
    lv_obj_set_style_text_color(vol_lbl, lv_color_hex(ui::Color::TEXT_SEC), 0);
    lv_obj_set_style_text_font(vol_lbl, ui::Theme::font_small(), 0);
    lv_obj_set_width(vol_lbl, 70);
    lv_obj_t* slider = lv_slider_create(vol_row);
    lv_obj_set_flex_grow(slider, 1);
    lv_obj_set_height(slider, 24);  // explicit height prevents flex missize
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, cfg.alarm_volume, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(ui::Color::SURFACE_2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(ui::Color::GOLD), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(ui::Color::GOLD_HI), LV_PART_KNOB);
    // Prevent horizontal drag from bubbling up as a scroll gesture on the panel
    lv_obj_remove_flag(slider, LV_OBJ_FLAG_GESTURE_BUBBLE);
    // Update the in-memory value on every drag tick (cheap), but only write NVS on release
    // (NVS flash writes are slow and cause watchdog resets if called ~60x/sec during drag)
    lv_obj_add_event_cb(slider, [](lv_event_t* e) {
        core::Settings::instance().get().alarm_volume = static_cast<uint8_t>(lv_slider_get_value(lv_event_get_target_obj(e)));
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(slider, [](lv_event_t* e) {
        core::Settings::instance().save();
    }, LV_EVENT_RELEASED, nullptr);
    lv_obj_t* vol_val = lv_label_create(vol_row);
    lv_obj_set_style_text_color(vol_val, lv_color_hex(ui::Color::GOLD), 0);
    lv_obj_set_style_text_font(vol_val, ui::Theme::font_small(), 0);
    lv_obj_set_width(vol_val, 40);
    char vol_str[8];
    std::snprintf(vol_str, sizeof(vol_str), "%d%%", cfg.alarm_volume);
    lv_label_set_text(vol_val, vol_str);
    lv_obj_add_event_cb(slider, [](lv_event_t* e) {
        lv_obj_t* lbl_ = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
        char s[8];
        std::snprintf(s, sizeof(s), "%d%%", static_cast<int>(lv_slider_get_value(lv_event_get_target_obj(e))));
        lv_label_set_text(lbl_, s);
    }, LV_EVENT_VALUE_CHANGED, vol_val);
}

void LauncherApp::on_unmount() {
    root_ = nullptr;
    clock_label_ = nullptr;
    date_label_ = nullptr;
    wifi_label_ = nullptr;
    battery_label_ = nullptr;
    greeting_label_ = nullptr;
}

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
        std::snprintf(battery, sizeof(battery), LV_SYMBOL_CHARGE " %u%%", static_cast<unsigned>(pct));
        lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::SUCCESS), 0);
    } else if (charge == core::ChargeState::FULL) {
        std::snprintf(battery, sizeof(battery), LV_SYMBOL_BATTERY_FULL " Full");
        lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::SUCCESS), 0);
    } else if (pct <= 15) {
        std::snprintf(battery, sizeof(battery), LV_SYMBOL_BATTERY_EMPTY " %u%%", static_cast<unsigned>(pct));
        lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::ERROR), 0);
    } else {
        std::snprintf(battery, sizeof(battery), LV_SYMBOL_BATTERY_3 " %u%%", static_cast<unsigned>(pct));
        lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::GOLD), 0);
    }
    lv_label_set_text(battery_label_, battery);

    const bool connected = core::Network::instance().is_connected();
    lv_label_set_text(wifi_label_, connected ? LV_SYMBOL_WIFI " " : "");
}

} // namespace apps
