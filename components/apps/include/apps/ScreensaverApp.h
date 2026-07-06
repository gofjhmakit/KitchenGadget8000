#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class ScreensaverApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::SCREENSAVER; }
    const char* name() const override { return "Screensaver"; }
    const char* icon() const override { return "☾"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    lv_obj_t* root_{nullptr};
    lv_obj_t* clock_label_{nullptr};
    lv_obj_t* date_label_{nullptr};
    lv_obj_t* day_label_{nullptr};
    lv_obj_t* battery_label_{nullptr};
    lv_obj_t* pulse_circle_{nullptr};
};

} // namespace apps
