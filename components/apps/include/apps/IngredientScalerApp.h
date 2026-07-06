#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class IngredientScalerApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::INGREDIENT_SCALER; }
    const char* name() const override { return "Scaler"; }
    const char* icon() const override { return "⚖"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    void recalc();
    lv_obj_t* root_{nullptr};
    lv_obj_t* amount_ta_{nullptr};
    lv_obj_t* servings_orig_ta_{nullptr};
    lv_obj_t* servings_target_ta_{nullptr};
    lv_obj_t* unit_dd_{nullptr};
    lv_obj_t* result_label_{nullptr};
};

} // namespace apps
