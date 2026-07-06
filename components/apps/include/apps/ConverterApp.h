#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class ConverterApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::CONVERTER; }
    const char* name() const override { return "Converter"; }
    const char* icon() const override { return "⇄"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;

    void build_ui(lv_obj_t* parent);
    void update_results();

    lv_obj_t* root_{nullptr};
    // Public so event callbacks can access them
    lv_obj_t* input_ta_{nullptr};
    lv_obj_t* from_dd_{nullptr};
    lv_obj_t* to_dd_{nullptr};

private:
    lv_obj_t* from_display_{nullptr};
    lv_obj_t* from_unit_display_{nullptr};
    lv_obj_t* result_label_{nullptr};
    lv_obj_t* to_unit_display_{nullptr};
};

} // namespace apps
