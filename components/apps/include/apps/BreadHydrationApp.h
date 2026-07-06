#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class BreadHydrationApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::BREAD_HYDRATION; }
    const char* name() const override { return "Hydration"; }
    const char* icon() const override { return "🍞"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    void recalc();
    lv_obj_t* root_{nullptr};
    lv_obj_t* flour_ta_{nullptr};
    lv_obj_t* hydration_ta_{nullptr};
    lv_obj_t* result_label_{nullptr};
};

} // namespace apps
