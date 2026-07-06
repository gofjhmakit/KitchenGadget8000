#include "apps/DishwasherApp.h"

#include <cstdio>
#include "core/Network.h"
#include "core/Notifications.h"
#include "core/Settings.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
const char* state_text(DishwasherApp::State state) {
    switch (state) {
        case DishwasherApp::State::RUNNING:  return "Running";
        case DishwasherApp::State::FINISHED: return "Finished";
        case DishwasherApp::State::ERROR:    return "Error";
        case DishwasherApp::State::IDLE:     return "Idle";
        default:                             return "Unknown";
    }
}

uint32_t state_color(DishwasherApp::State state) {
    switch (state) {
        case DishwasherApp::State::RUNNING:  return ui::Color::GOLD_HI;
        case DishwasherApp::State::FINISHED: return ui::Color::SUCCESS;
        case DishwasherApp::State::ERROR:    return ui::Color::ERROR;
        default:                             return ui::Color::TEXT_SEC;
    }
}

std::string find_json_value(const std::string& json, const char* key) {
    const std::string token = std::string("\"") + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return {};
    pos += token.size();
    const auto end = json.find('"', pos);
    return end == std::string::npos ? std::string{} : json.substr(pos, end - pos);
}

lv_obj_t* make_info_row(lv_obj_t* parent, const char* label_txt, const char* init_value, lv_obj_t** value_out) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_ver(row, ui::Spacing::SM, 0);
    // Bottom separator line
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(ui::Color::SURFACE_3), 0);

    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, label_txt);
    lv_obj_set_style_text_font(lbl, ui::Theme::font_small(), 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(ui::Color::TEXT_HINT), 0);

    lv_obj_t* val = lv_label_create(row);
    lv_label_set_text(val, init_value);
    lv_obj_set_style_text_font(val, ui::Theme::font_small(), 0);
    lv_obj_set_style_text_color(val, lv_color_hex(ui::Color::TEXT_PRI), 0);
    if (value_out) *value_out = val;
    return row;
}
} // anonymous namespace

void DishwasherApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); refresh(); }

void DishwasherApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* outer = lv_obj_create(parent);
    lv_obj_remove_style_all(outer);
    lv_obj_set_size(outer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(outer, ui::Spacing::LG, 0);
    lv_obj_set_layout(outer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(outer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(outer, ui::Spacing::MD, 0);

    // Header
    ui::create_app_header(outer, "DISHWASHER", "Refresh",
        [](lv_event_t* e){ static_cast<DishwasherApp*>(lv_event_get_user_data(e))->refresh(); }, this);

    // Main body: left arc + right info table
    lv_obj_t* body = lv_obj_create(outer);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(body, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(body, ui::Spacing::XL, 0);

    // Left column
    lv_obj_t* left = lv_obj_create(body);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, 280, LV_PCT(100));
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(left, ui::Spacing::SM, 0);

    arc_ = ui::create_progress_ring(left, ui::Color::GOLD_HI, 200);
    lv_arc_set_value(arc_, 0);

    status_label_ = lv_label_create(left);
    lv_label_set_text(status_label_, "—");
    lv_obj_set_style_text_font(status_label_, ui::Theme::font_title(), 0);
    lv_obj_set_style_text_color(status_label_, lv_color_hex(ui::Color::GOLD_HI), 0);

    time_label_ = lv_label_create(left);
    lv_label_set_text(time_label_, "00:00");
    lv_obj_set_style_text_font(time_label_, ui::Theme::font_body(), 0);
    lv_obj_set_style_text_color(time_label_, lv_color_hex(ui::Color::TEXT_SEC), 0);

    pause_btn_ = ui::create_gold_button(left, "PAUSE");
    lv_obj_set_width(pause_btn_, 160);

    // Right column: info table card
    lv_obj_t* right = ui::create_card(body);
    lv_obj_set_size(right, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 0, 0);

    lv_obj_t* rtitle = ui::create_section_title(right, "DETAILS");
    (void)rtitle;
    make_info_row(right, "Program",    "—",   &program_value_);
    make_info_row(right, "Temperature","—",   &temp_value_);
    make_info_row(right, "Rinse aid",  "—",   &rinse_value_);
    make_info_row(right, "Salt",       "—",   &salt_value_);
}

void DishwasherApp::refresh() {
    const auto& settings = core::Settings::instance().get();
    bool has_token = settings.hc_access_token[0] != '\0';
    std::string body;
    std::string program = "—", temp_str = "—", rinse = "OK", salt = "OK";

    if (!has_token || !core::Network::instance().is_connected()) {
        state_ = State::IDLE;
        remaining_sec_ = 0;
        total_sec_ = 0;
        program = has_token ? "Offline" : "No token";
    } else {
        const std::string appliance = settings.hc_appliance_id[0] ? settings.hc_appliance_id : "";
        const std::string url = appliance.empty()
            ? "https://api.home-connect.com/api/homeappliances"
            : "https://api.home-connect.com/api/homeappliances/" + appliance + "/status";
        const bool ok = core::Network::instance().http_get(url, body,
            {{"Authorization", std::string("Bearer ") + settings.hc_access_token}});
        if (ok) {
            if (body.find("Run") != std::string::npos) {
                state_ = State::RUNNING;
                total_sec_ = 7200;
                remaining_sec_ = remaining_sec_ == 0 ? 5400 : remaining_sec_;
                program = "Eco 50°C";
                temp_str = "50°C";
            } else if (body.find("Finished") != std::string::npos) {
                if (state_ != State::FINISHED)
                    core::Notifications::instance().push(core::NotificationType::SUCCESS, "Dishwasher finished", "Dishes are clean.");
                state_ = State::FINISHED;
                remaining_sec_ = 0;
                total_sec_ = 1;
                program = "Complete";
            } else if (body.find("Error") != std::string::npos) {
                state_ = State::ERROR;
                program = "Error — check appliance";
            } else {
                state_ = State::IDLE;
                program = "Waiting";
            }
        } else {
            state_ = State::UNKNOWN;
            program = "API unavailable";
        }
    }

    // Update left panel
    lv_label_set_text(status_label_, state_text(state_));
    lv_obj_set_style_text_color(status_label_, lv_color_hex(state_color(state_)), 0);
    char time_buf[16];
    std::snprintf(time_buf, sizeof(time_buf), "%02u:%02u", remaining_sec_ / 60, remaining_sec_ % 60);
    lv_label_set_text(time_label_, time_buf);
    lv_arc_set_value(arc_, total_sec_ == 0 ? 0 : static_cast<int>((total_sec_ - remaining_sec_) * 100 / total_sec_));

    // Update right info table
    if (program_value_) lv_label_set_text(program_value_, program.c_str());
    if (temp_value_)    lv_label_set_text(temp_value_, temp_str.c_str());
    if (rinse_value_)   lv_label_set_text(rinse_value_, rinse.c_str());
    if (salt_value_)    lv_label_set_text(salt_value_, salt.c_str());

    program_str_ = program;
    refresh_accumulator_ = 0.0f;
    second_accumulator_ = 0.0f;
}

void DishwasherApp::on_unmount() {
    root_ = nullptr;
    arc_ = nullptr; status_label_ = nullptr; time_label_ = nullptr; pause_btn_ = nullptr;
    program_value_ = nullptr; temp_value_ = nullptr; rinse_value_ = nullptr; salt_value_ = nullptr;
}

void DishwasherApp::on_update(float delta_sec) {
    refresh_accumulator_ += delta_sec;
    second_accumulator_ += delta_sec;

    if (state_ == State::RUNNING && remaining_sec_ > 0) {
        while (second_accumulator_ >= 1.0f && remaining_sec_ > 0) {
            second_accumulator_ -= 1.0f;
            remaining_sec_ = remaining_sec_ > 1 ? remaining_sec_ - 1 : 0;
            if (time_label_) {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%02u:%02u", remaining_sec_ / 60, remaining_sec_ % 60);
                lv_label_set_text(time_label_, buf);
            }
            if (arc_ && total_sec_ > 0)
                lv_arc_set_value(arc_, static_cast<int>((total_sec_ - remaining_sec_) * 100 / total_sec_));
            if (remaining_sec_ == 0) {
                state_ = State::FINISHED;
                core::Notifications::instance().push(core::NotificationType::SUCCESS, "Dishwasher finished", "Cycle complete.");
            }
        }
    }
    if (refresh_accumulator_ >= 20.0f) refresh();
}

} // namespace apps

