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
public:
    enum class Category { VOLUME, WEIGHT, TEMPERATURE };
    void build_ui(lv_obj_t* parent);
    void update_results();
    lv_obj_t* root_{nullptr};
    lv_obj_t* input_ta_{nullptr};
    lv_obj_t* result_container_{nullptr};
    lv_obj_t* result_labels_[9]{};
    Category category_{Category::VOLUME};
};

} // namespace apps
