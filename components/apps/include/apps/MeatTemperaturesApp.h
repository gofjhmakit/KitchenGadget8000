#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class MeatTemperaturesApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::MEAT_TEMPERATURES; }
    const char* name() const override { return "Meat Temps"; }
    const char* icon() const override { return "🥩"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    lv_obj_t* root_{nullptr};
};

} // namespace apps
