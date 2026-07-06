#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class LauncherApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::LAUNCHER; }
    const char* name() const override { return "Launcher"; }
    const char* icon() const override { return "⌂"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;

    void show_home(lv_obj_t* parent);
    void show_all_apps(lv_obj_t* parent);

    lv_obj_t* root_{nullptr};

private:
    lv_obj_t* clock_label_{nullptr};
    lv_obj_t* date_label_{nullptr};
    lv_obj_t* battery_label_{nullptr};
    lv_obj_t* wifi_label_{nullptr};
    lv_obj_t* greeting_label_{nullptr};
};

} // namespace apps
