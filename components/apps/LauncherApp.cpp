#include "apps/LauncherApp.h"

#include <array>
#include <cstdio>
#include "core/Navigation.h"
#include "core/Network.h"
#include "core/PowerManager.h"
#include "services/TimeService.h"
#include "ui/Theme.h"

namespace apps {
namespace {
struct AppEntry { core::AppId id; const char* name; const char* icon; };
constexpr std::array<AppEntry, 12> kApps{{
    {core::AppId::TIMERS, "Timers", "⏱"},
    {core::AppId::CONVERTER, "Converter", "⇄"},
    {core::AppId::INGREDIENT_SCALER, "Scaler", "⚖"},
    {core::AppId::BREAD_HYDRATION, "Hydration", "🍞"},
    {core::AppId::DRINK_CHILLER, "Chiller", "🧊"},
    {core::AppId::MEAT_TEMPERATURES, "Meat Temps", "🥩"},
    {core::AppId::DISHWASHER, "Dishwasher", "🫧"},
    {core::AppId::WEATHER, "Weather", "☀"},
    {core::AppId::RECIPES, "Recipes", "📖"},
    {core::AppId::LIGHTING, "Lighting", "💡"},
    {core::AppId::SHOPPING_LIST, "Shopping", "🛒"},
    {core::AppId::NOTES, "Notes", "✎"},
}};

void launch_app(lv_event_t* e) {
    auto* entry = static_cast<const AppEntry*>(lv_event_get_user_data(e));
    core::PowerManager::instance().reset_activity();
    core::Navigation::instance().navigate_to(entry->id, core::AppManager::Transition::SLIDE_LEFT);
}
}

void LauncherApp::on_mount(lv_obj_t* parent) {
    root_ = parent;
    build_ui(parent);
}

void LauncherApp::build_ui(lv_obj_t* parent) {
    lv_obj_set_style_bg_color(parent, lv_color_hex(ui::Color::BG), 0);
    lv_obj_t* main = lv_obj_create(parent);
    lv_obj_remove_style_all(main);
    lv_obj_set_size(main, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(main, ui::Spacing::LG, 0);
    lv_obj_set_layout(main, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* header = lv_obj_create(main);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, LV_PCT(100), 88);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* title_col = lv_obj_create(header);
    lv_obj_remove_style_all(title_col);
    lv_obj_set_layout(title_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(title_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_t* title = lv_label_create(title_col);
    lv_label_set_text(title, "KitchenGadget8000");
    lv_obj_set_style_text_font(title, ui::Theme::font_title(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(ui::Color::GOLD_HI), 0);
    date_label_ = lv_label_create(title_col);
    lv_obj_set_style_text_color(date_label_, lv_color_hex(ui::Color::TEXT_SEC), 0);

    lv_obj_t* status_col = lv_obj_create(header);
    lv_obj_remove_style_all(status_col);
    lv_obj_set_layout(status_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    clock_label_ = lv_label_create(status_col);
    lv_obj_set_style_text_font(clock_label_, ui::Theme::font_title(), 0);
    lv_obj_set_style_text_color(clock_label_, lv_color_hex(ui::Color::TEXT_PRI), 0);

    lv_obj_t* mini = lv_obj_create(status_col);
    lv_obj_remove_style_all(mini);
    lv_obj_set_layout(mini, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mini, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(mini, ui::Spacing::MD, 0);
    wifi_label_ = lv_label_create(mini);
    battery_label_ = lv_label_create(mini);
    lv_obj_set_style_text_color(wifi_label_, lv_color_hex(ui::Color::GOLD), 0);
    lv_obj_set_style_text_color(battery_label_, lv_color_hex(ui::Color::GOLD), 0);

    lv_obj_t* grid = lv_obj_create(main);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100));
    static int32_t cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t rows[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, cols, rows);
    lv_obj_set_style_pad_row(grid, ui::Spacing::LG, 0);
    lv_obj_set_style_pad_column(grid, ui::Spacing::LG, 0);

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
        lv_obj_set_style_text_font(icon, ui::Theme::font_title(), 0);
        lv_obj_t* name = lv_label_create(card);
        lv_label_set_text(name, kApps[i].name);
        lv_obj_set_style_text_font(name, ui::Theme::font_body(), 0);
        lv_obj_set_style_text_color(name, lv_color_hex(ui::Color::GOLD), 0);
    }
    on_update(0.0f);
}

void LauncherApp::on_unmount() { root_ = nullptr; }

void LauncherApp::on_update(float) {
    if (root_ == nullptr) return;
    lv_label_set_text(clock_label_, services::TimeService::instance().time_string(false).c_str());
    lv_label_set_text(date_label_, services::TimeService::instance().date_string().c_str());
    char battery[32];
    std::snprintf(battery, sizeof(battery), "🔋 %u%%", core::PowerManager::instance().battery_percent());
    lv_label_set_text(battery_label_, battery);
    const bool connected = core::Network::instance().is_connected();
    lv_label_set_text(wifi_label_, connected ? "📶 Online" : "📡 Offline");
}

} // namespace apps
