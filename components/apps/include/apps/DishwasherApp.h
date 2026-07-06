#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class DishwasherApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::DISHWASHER; }
    const char* name() const override { return "Dishwasher"; }
    const char* icon() const override { return "🫧"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    enum class State { RUNNING, FINISHED, ERROR, IDLE, UNKNOWN };
    void build_ui(lv_obj_t* parent);
    void refresh();
    lv_obj_t* root_{nullptr};
    lv_obj_t* status_label_{nullptr};
    lv_obj_t* program_label_{nullptr};
    lv_obj_t* time_label_{nullptr};
    lv_obj_t* arc_{nullptr};
    lv_obj_t* refresh_label_{nullptr};
    State state_{State::UNKNOWN};
    uint32_t remaining_sec_{0};
    uint32_t total_sec_{0};
    float refresh_accumulator_{0.0f};
    float second_accumulator_{0.0f};
};

} // namespace apps
