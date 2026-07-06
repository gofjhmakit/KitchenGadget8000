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
        case DishwasherApp::State::RUNNING: return "Running";
        case DishwasherApp::State::FINISHED: return "Finished";
        case DishwasherApp::State::ERROR: return "Error";
        case DishwasherApp::State::IDLE: return "Idle";
        default: return "Unknown";
    }
}

uint32_t state_color(DishwasherApp::State state) {
    switch (state) {
        case DishwasherApp::State::RUNNING: return ui::Color::GOLD_HI;
        case DishwasherApp::State::FINISHED: return ui::Color::SUCCESS;
        case DishwasherApp::State::ERROR: return ui::Color::ERROR;
        case DishwasherApp::State::IDLE: return ui::Color::TEXT_SEC;
        default: return ui::Color::WARNING;
    }
}

std::string find_json_value(const std::string& json, const char* key) {
    const std::string token = std::string("\"") + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return {};
    pos += token.size();
    auto end = json.find('"', pos);
    return end == std::string::npos ? std::string{} : json.substr(pos, end - pos);
}

void refresh_click(lv_event_t* e) { static_cast<DishwasherApp*>(lv_event_get_user_data(e))->refresh(); }
}

void DishwasherApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); refresh(); }

void DishwasherApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(row, ui::Spacing::LG, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, ui::Spacing::XL, 0);

    lv_obj_t* left = ui::create_card(row);
    lv_obj_set_size(left, 380, LV_PCT(100));
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(left, "Dishwasher");
    status_label_ = lv_label_create(left);
    lv_obj_set_style_text_font(status_label_, ui::Theme::font_huge(), 0);
    program_label_ = lv_label_create(left);
    time_label_ = lv_label_create(left);
    lv_obj_set_style_text_font(time_label_, ui::Theme::font_title(), 0);
    lv_obj_t* refresh = ui::create_gold_button(left, "Refresh");
    lv_obj_add_event_cb(refresh, refresh_click, LV_EVENT_CLICKED, this);
    refresh_label_ = lv_label_create(left);

    lv_obj_t* right = ui::create_card(row);
    lv_obj_set_size(right, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    arc_ = ui::create_progress_ring(right, ui::Color::GOLD_HI, 260);
    lv_arc_set_value(arc_, 0);
}

void DishwasherApp::refresh() {
    const auto& settings = core::Settings::instance().get();
    std::string body;
    bool has_token = settings.hc_access_token[0] != '\0';
    if (!has_token || !core::Network::instance().is_connected()) {
        state_ = State::IDLE;
        remaining_sec_ = 0;
        total_sec_ = 0;
        lv_label_set_text(program_label_, has_token ? "Offline demo" : "Configure Home Connect token");
    } else {
        const std::string appliance = settings.hc_appliance_id[0] ? settings.hc_appliance_id : "";
        const std::string url = appliance.empty()
            ? "https://api.home-connect.com/api/homeappliances"
            : "https://api.home-connect.com/api/homeappliances/" + appliance + "/status";
        const bool ok = core::Network::instance().http_get(url, body, {{"Authorization", std::string("Bearer ") + settings.hc_access_token}});
        if (ok) {
            if (body.find("Run") != std::string::npos || body.find("BSH.Common.EnumType.OperationState.Run") != std::string::npos) {
                state_ = State::RUNNING;
                total_sec_ = 7200;
                remaining_sec_ = remaining_sec_ == 0 ? 5400 : remaining_sec_;
                lv_label_set_text(program_label_, "Eco 50°C");
            } else if (body.find("Finished") != std::string::npos) {
                if (state_ != State::FINISHED) {
                    core::Notifications::instance().push(core::NotificationType::SUCCESS, "Dishwasher finished", "Dishes are clean and ready.");
                }
                state_ = State::FINISHED;
                remaining_sec_ = 0;
                total_sec_ = 1;
                lv_label_set_text(program_label_, "Cycle complete");
            } else if (body.find("Error") != std::string::npos) {
                state_ = State::ERROR;
                lv_label_set_text(program_label_, "Check appliance");
            } else {
                state_ = State::IDLE;
                lv_label_set_text(program_label_, "Waiting for program");
            }
        } else {
            state_ = State::UNKNOWN;
            lv_label_set_text(program_label_, "API unavailable");
        }
    }
    lv_label_set_text(status_label_, state_text(state_));
    lv_obj_set_style_text_color(status_label_, lv_color_hex(state_color(state_)), 0);
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%02u:%02u remaining", remaining_sec_ / 60, remaining_sec_ % 60);
    lv_label_set_text(time_label_, buf);
    lv_arc_set_value(arc_, total_sec_ == 0 ? 0 : static_cast<int>((total_sec_ - remaining_sec_) * 100 / total_sec_));
    lv_label_set_text(refresh_label_, "Auto refresh every 20 s");
    refresh_accumulator_ = 0.0f;
    second_accumulator_ = 0.0f;
}

void DishwasherApp::on_unmount() { root_ = nullptr; }

void DishwasherApp::on_update(float delta_sec) {
    refresh_accumulator_ += delta_sec;
    second_accumulator_ += delta_sec;
    if (state_ == State::RUNNING && remaining_sec_ > 0) {
        while (second_accumulator_ >= 1.0f && remaining_sec_ > 0) {
            second_accumulator_ -= 1.0f;
            remaining_sec_ = remaining_sec_ > 1 ? remaining_sec_ - 1 : 0;
            char buf[48];
            std::snprintf(buf, sizeof(buf), "%02u:%02u remaining", remaining_sec_ / 60, remaining_sec_ % 60);
            lv_label_set_text(time_label_, buf);
            lv_arc_set_value(arc_, total_sec_ == 0 ? 0 : static_cast<int>((total_sec_ - remaining_sec_) * 100 / total_sec_));
            if (remaining_sec_ == 0) {
                state_ = State::FINISHED;
                core::Notifications::instance().push(core::NotificationType::SUCCESS, "Dishwasher finished", "Cycle complete.");
            }
        }
    }
    if (refresh_accumulator_ >= 20.0f) refresh();
}

} // namespace apps
