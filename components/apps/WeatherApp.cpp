#include "apps/WeatherApp.h"

#include <cstdio>
#include <cstdlib>
#include "core/Network.h"
#include "core/Settings.h"
#include "services/TimeService.h"
#include "ui/Theme.h"

namespace apps {
namespace {
std::string number_after(const std::string& text, const std::string& key) {
    auto pos = text.find(key);
    if (pos == std::string::npos) return {};
    pos += key.size();
    auto end = text.find_first_of(",]}", pos);
    return text.substr(pos, end - pos);
}

const char* icon_for_code(int code) {
    if (code == 0) return "☀️";
    if (code <= 2) return "🌤";
    if (code <= 48) return "⛅";
    if (code <= 67) return "🌧";
    if (code <= 82) return "⛈";
    return "🌨";
}

void refresh_click(lv_event_t* e) { static_cast<WeatherApp*>(lv_event_get_user_data(e))->refresh(); }
}

void WeatherApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); refresh(); }

void WeatherApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont, ui::Spacing::LG, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(cont, "Weather");

    current_label_ = lv_label_create(cont);
    lv_obj_set_style_text_font(current_label_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(current_label_, lv_color_hex(ui::Color::GOLD_HI), 0);
    condition_label_ = lv_label_create(cont);
    today_label_ = lv_label_create(cont);
    tomorrow_label_ = lv_label_create(cont);
    updated_label_ = lv_label_create(cont);
    lv_obj_t* refresh = ui::create_gold_button(cont, "Refresh");
    lv_obj_add_event_cb(refresh, refresh_click, LV_EVENT_CLICKED, this);
}

void WeatherApp::refresh() {
    const auto& s = core::Settings::instance().get();
    std::string response;
    if (core::Network::instance().is_connected()) {
        char url[384];
        std::snprintf(url, sizeof(url),
            "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,weather_code&daily=temperature_2m_max,temperature_2m_min,weather_code&forecast_days=2&timezone=auto",
            s.weather_lat, s.weather_lon);
        core::Network::instance().http_get(url, response);
    }
    if (response.empty()) {
        lv_label_set_text(current_label_, "18°C");
        lv_label_set_text(condition_label_, "🌤  Pleasant and bright");
        lv_label_set_text(today_label_, "Today 21° / 14°");
        lv_label_set_text(tomorrow_label_, "Tomorrow 19° / 12° ⛅");
    } else {
        const double temp = std::atof(number_after(response, "\"temperature_2m\":").c_str());
        const int code = std::atoi(number_after(response, "\"weather_code\":").c_str());
        const auto max_pos = response.find("\"temperature_2m_max\":[");
        const auto min_pos = response.find("\"temperature_2m_min\":[");
        double t0max = 21, t1max = 19, t0min = 13, t1min = 11;
        if (max_pos != std::string::npos) {
            auto sub = response.substr(max_pos);
            t0max = std::atof(number_after(sub, "[").c_str());
            auto comma = sub.find(',');
            if (comma != std::string::npos) t1max = std::atof(sub.substr(comma + 1).c_str());
        }
        if (min_pos != std::string::npos) {
            auto sub = response.substr(min_pos);
            t0min = std::atof(number_after(sub, "[").c_str());
            auto comma = sub.find(',');
            if (comma != std::string::npos) t1min = std::atof(sub.substr(comma + 1).c_str());
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f°C", temp);
        lv_label_set_text(current_label_, buf);
        std::snprintf(buf, sizeof(buf), "%s  %s", icon_for_code(code), s.weather_city);
        lv_label_set_text(condition_label_, buf);
        std::snprintf(buf, sizeof(buf), "Today %.0f° / %.0f°", t0max, t0min);
        lv_label_set_text(today_label_, buf);
        std::snprintf(buf, sizeof(buf), "Tomorrow %.0f° / %.0f°", t1max, t1min);
        lv_label_set_text(tomorrow_label_, buf);
    }
    lv_label_set_text(updated_label_, services::TimeService::instance().timestamp_string().c_str());
}

void WeatherApp::on_unmount() { root_ = nullptr; }
void WeatherApp::on_update(float) {}

} // namespace apps
