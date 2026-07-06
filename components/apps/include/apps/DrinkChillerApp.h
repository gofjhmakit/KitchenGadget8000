#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class DrinkChillerApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::DRINK_CHILLER; }
    const char* name() const override { return "Chiller"; }
    const char* icon() const override { return "🧊"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    void recalc();
    lv_obj_t* root_{nullptr};
    lv_obj_t* container_dd_{nullptr};
    lv_obj_t* start_slider_{nullptr};
    lv_obj_t* target_slider_{nullptr};
    lv_obj_t* location_dd_{nullptr};
    lv_obj_t* result_label_{nullptr};
    lv_obj_t* start_value_{nullptr};
    lv_obj_t* target_value_{nullptr};
};

} // namespace apps
