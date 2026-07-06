#include "apps/WeatherApp.h"

#include <cstdio>
#include <cstdlib>
#include "core/Network.h"
#include "core/Settings.h"
#include "services/TimeService.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

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
    if (code == 0) return "☀";
    if (code <= 2) return "⛅";
    if (code <= 48) return "☁";
    if (code <= 67) return "🌧";
    if (code <= 82) return "⛈";
    return "🌨";
}

const char* day_short(int offset) {
    static const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    // offset 0 = today, use TimeService for day-of-week
    // We just label relative days for simplicity
    static const char* rel[] = {"Today","Tom.","Wed.","Thu.","Fri."};
    if (offset < 5) return rel[offset];
    return days[offset % 7];
}
} // anonymous namespace

void WeatherApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); refresh(); }

void WeatherApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* outer = lv_obj_create(parent);
    lv_obj_remove_style_all(outer);
    lv_obj_set_size(outer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(outer, ui::Spacing::LG, 0);
    lv_obj_set_layout(outer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(outer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(outer, ui::Spacing::MD, 0);

    // Header: WEATHER + city label
    lv_obj_t* hdr = lv_obj_create(outer);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, LV_PCT(100), 44);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* hdr_title = lv_label_create(hdr);
    lv_label_set_text(hdr_title, "WEATHER");
    lv_obj_set_style_text_font(hdr_title, ui::Theme::font_heading(), 0);
    lv_obj_set_style_text_color(hdr_title, lv_color_hex(ui::Color::GOLD_HI), 0);
    city_label_ = lv_label_create(hdr);
    lv_obj_set_style_text_color(city_label_, lv_color_hex(ui::Color::TEXT_SEC), 0);
    lv_obj_set_style_text_font(city_label_, ui::Theme::font_label(), 0);

    // Main row: left (icon+temp) | right (stats)
    lv_obj_t* main_row = lv_obj_create(outer);
    lv_obj_remove_style_all(main_row);
    lv_obj_set_size(main_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(main_row, 200, 0);
    lv_obj_set_layout(main_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(main_row, ui::Spacing::LG, 0);

    // Left: large icon + huge temp + condition
    lv_obj_t* left = lv_obj_create(main_row);
    lv_obj_remove_style_all(left);
    lv_obj_set_flex_grow(left, 3);
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(left, ui::Spacing::SM, 0);

    weather_icon_label_ = lv_label_create(left);
    lv_obj_set_style_text_font(weather_icon_label_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(weather_icon_label_, lv_color_hex(ui::Color::GOLD_HI), 0);

    current_label_ = lv_label_create(left);
    lv_obj_set_style_text_font(current_label_, ui::Theme::font_huge(), 0);
    lv_obj_set_style_text_color(current_label_, lv_color_hex(ui::Color::GOLD_HI), 0);

    condition_label_ = lv_label_create(left);
    lv_obj_set_style_text_font(condition_label_, ui::Theme::font_body(), 0);
    lv_obj_set_style_text_color(condition_label_, lv_color_hex(ui::Color::TEXT_SEC), 0);

    // Right: stat rows (feels like, humidity, wind)
    lv_obj_t* right = ui::create_card(main_row);
    lv_obj_set_flex_grow(right, 2);
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(right, ui::Spacing::MD, 0);

    auto make_stat_row = [](lv_obj_t* parent, const char* icon, const char* label_text, lv_obj_t** value_out) {
        lv_obj_t* row = lv_obj_create(parent);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t* lbl = lv_label_create(row);
        char buf[32]; std::snprintf(buf, sizeof(buf), "%s  %s", icon, label_text);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_color(lbl, lv_color_hex(ui::Color::TEXT_HINT), 0);
        lv_obj_set_style_text_font(lbl, ui::Theme::font_small(), 0);
        *value_out = lv_label_create(row);
        lv_obj_set_style_text_color(*value_out, lv_color_hex(ui::Color::TEXT_SEC), 0);
        lv_obj_set_style_text_font(*value_out, ui::Theme::font_body(), 0);
    };
    make_stat_row(right, LV_SYMBOL_TINT, "Feels like", &feels_like_label_);
    make_stat_row(right, "~",            "Humidity",   &humidity_label_);
    make_stat_row(right, LV_SYMBOL_RIGHT,"Wind",       &wind_label_);

    // 5-day forecast row
    lv_obj_t* forecast_row = lv_obj_create(outer);
    lv_obj_remove_style_all(forecast_row);
    lv_obj_set_size(forecast_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(forecast_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(forecast_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(forecast_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(forecast_row, ui::Spacing::SM, 0);

    for (int i = 0; i < 5; ++i) {
        lv_obj_t* day_card = ui::create_card(forecast_row);
        lv_obj_set_flex_grow(day_card, 1);
        lv_obj_set_layout(day_card, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(day_card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(day_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(day_card, ui::Spacing::XS, 0);

        forecast_day_labels_[i] = lv_label_create(day_card);
        lv_label_set_text(forecast_day_labels_[i], day_short(i));
        lv_obj_set_style_text_font(forecast_day_labels_[i], ui::Theme::font_small(), 0);
        lv_obj_set_style_text_color(forecast_day_labels_[i], lv_color_hex(ui::Color::TEXT_HINT), 0);

        forecast_icon_labels_[i] = lv_label_create(day_card);
        lv_obj_set_style_text_font(forecast_icon_labels_[i], ui::Theme::font_body(), 0);
        lv_obj_set_style_text_color(forecast_icon_labels_[i], lv_color_hex(ui::Color::GOLD), 0);

        forecast_temp_labels_[i] = lv_label_create(day_card);
        lv_obj_set_style_text_font(forecast_temp_labels_[i], ui::Theme::font_small(), 0);
        lv_obj_set_style_text_color(forecast_temp_labels_[i], lv_color_hex(ui::Color::TEXT_SEC), 0);
    }

    updated_label_ = lv_label_create(outer);
    lv_obj_set_style_text_font(updated_label_, ui::Theme::font_small(), 0);
    lv_obj_set_style_text_color(updated_label_, lv_color_hex(ui::Color::TEXT_HINT), 0);
}

void WeatherApp::refresh() {
    const auto& s = core::Settings::instance().get();
    std::string response;
    if (core::Network::instance().is_connected()) {
        char url[512];
        std::snprintf(url, sizeof(url),
            "https://api.open-meteo.com/v1/forecast"
            "?latitude=%.4f&longitude=%.4f"
            "&current=temperature_2m,apparent_temperature,weather_code,wind_speed_10m,relative_humidity_2m"
            "&daily=temperature_2m_max,temperature_2m_min,weather_code"
            "&forecast_days=5&timezone=auto",
            s.weather_lat, s.weather_lon);
        core::Network::instance().http_get(url, response);
    }

    // City label
    char city_buf[64];
    std::snprintf(city_buf, sizeof(city_buf), "%s " LV_SYMBOL_GPS, s.weather_city);
    lv_label_set_text(city_label_, city_buf);

    if (response.empty()) {
        lv_label_set_text(weather_icon_label_, "⛅");
        lv_label_set_text(current_label_, "18°C");
        lv_label_set_text(condition_label_, "Partly Cloudy");
        lv_label_set_text(feels_like_label_, "16°C");
        lv_label_set_text(humidity_label_, "55%");
        lv_label_set_text(wind_label_, "3 m/s");
        for (int i = 0; i < 5; ++i) {
            lv_label_set_text(forecast_icon_labels_[i], "⛅");
            lv_label_set_text(forecast_temp_labels_[i], "--/--");
        }
    } else {
        const double temp = std::atof(number_after(response, "\"temperature_2m\":").c_str());
        const double feels = std::atof(number_after(response, "\"apparent_temperature\":").c_str());
        const int code = std::atoi(number_after(response, "\"weather_code\":").c_str());
        const double wind = std::atof(number_after(response, "\"wind_speed_10m\":").c_str());
        const double humid = std::atof(number_after(response, "\"relative_humidity_2m\":").c_str());

        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f°C", temp);
        lv_label_set_text(current_label_, buf);
        lv_label_set_text(weather_icon_label_, icon_for_code(code));
        // Derive condition text from WMO code
        const char* condition = (code == 0) ? "Clear Sky" : (code <= 2) ? "Mostly Clear" :
                                (code <= 48) ? "Cloudy" : (code <= 67) ? "Rain" :
                                (code <= 82) ? "Thunderstorm" : "Snow";
        lv_label_set_text(condition_label_, condition);

        std::snprintf(buf, sizeof(buf), "%.0f°C", feels);
        lv_label_set_text(feels_like_label_, buf);
        std::snprintf(buf, sizeof(buf), "%.0f%%", humid);
        lv_label_set_text(humidity_label_, buf);
        std::snprintf(buf, sizeof(buf), "%.1f m/s", wind);
        lv_label_set_text(wind_label_, buf);

        // Parse 5-day forecast
        const auto max_pos = response.find("\"temperature_2m_max\":[");
        const auto min_pos = response.find("\"temperature_2m_min\":[");
        const auto wc_pos  = response.find("\"weather_code\":["); // daily weather_code array

        auto parse_array = [&](size_t start_pos, const char* marker, double out[5]) {
            if (start_pos == std::string::npos) return;
            auto sub = response.substr(start_pos);
            auto arr_start = sub.find('[');
            if (arr_start == std::string::npos) return;
            sub = sub.substr(arr_start + 1);
            for (int i = 0; i < 5; ++i) {
                out[i] = std::atof(sub.c_str());
                auto comma = sub.find(',');
                if (comma == std::string::npos) break;
                sub = sub.substr(comma + 1);
            }
        };
        double max5[5]{}, min5[5]{};
        parse_array(max_pos, "max", max5);
        parse_array(min_pos, "min", min5);

        // Daily weather codes
        double wc5[5]{};
        if (wc_pos != std::string::npos) parse_array(wc_pos, "wc", wc5);

        for (int i = 0; i < 5; ++i) {
            lv_label_set_text(forecast_icon_labels_[i], icon_for_code(static_cast<int>(wc5[i])));
            char tbuf[24];
            std::snprintf(tbuf, sizeof(tbuf), "%.0f°/%.0f°", max5[i], min5[i]);
            lv_label_set_text(forecast_temp_labels_[i], tbuf);
        }
    }
    lv_label_set_text(updated_label_, services::TimeService::instance().timestamp_string().c_str());
    refresh_accumulator_ = 0.0f;
}

void WeatherApp::on_unmount() { root_ = nullptr; }

void WeatherApp::on_update(float delta_sec) {
    refresh_accumulator_ += delta_sec;
    if (refresh_accumulator_ >= 300.0f) refresh(); // auto-refresh every 5 minutes
}

} // namespace apps

