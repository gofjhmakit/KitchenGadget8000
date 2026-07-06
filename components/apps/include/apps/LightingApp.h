#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class LightingApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::LIGHTING; }
    const char* name() const override { return "Lighting"; }
    const char* icon() const override { return "💡"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    void apply_scene(const char* scene);
    lv_obj_t* root_{nullptr};
    lv_obj_t* power_switch_{nullptr};
    lv_obj_t* brightness_slider_{nullptr};
    lv_obj_t* temp_slider_{nullptr};
    lv_obj_t* scene_label_{nullptr};
};

} // namespace apps
