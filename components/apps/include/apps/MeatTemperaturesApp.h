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

    void build_ui(lv_obj_t* parent);
    void select(int idx);

    lv_obj_t* root_{nullptr};

private:
    static constexpr int kMeatCount = 5;
    lv_obj_t* meat_btns_[kMeatCount]{};
    lv_obj_t* detail_card_{nullptr};
    lv_obj_t* detail_doneness_{nullptr};
    lv_obj_t* detail_animal_{nullptr};
    lv_obj_t* detail_temp_c_{nullptr};
    lv_obj_t* detail_temp_f_{nullptr};
    lv_obj_t* detail_tip_{nullptr};
    int selected_{0};
};

} // namespace apps
