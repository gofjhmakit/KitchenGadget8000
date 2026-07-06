#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class WeatherApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::WEATHER; }
    const char* name() const override { return "Weather"; }
    const char* icon() const override { return "☀"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    void refresh();
    lv_obj_t* root_{nullptr};
    lv_obj_t* current_label_{nullptr};
    lv_obj_t* condition_label_{nullptr};
    lv_obj_t* today_label_{nullptr};
    lv_obj_t* tomorrow_label_{nullptr};
    lv_obj_t* updated_label_{nullptr};
};

} // namespace apps
