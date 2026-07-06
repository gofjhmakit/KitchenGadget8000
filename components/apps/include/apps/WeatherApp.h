#pragma once
#include "core/AppManager.h"
#include "lvgl.h"

namespace apps {

class WeatherApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::WEATHER; }
    const char* name() const override { return "Weather"; }
    const char* icon() const override { return "☀"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;

    void build_ui(lv_obj_t* parent);
    void refresh();

    lv_obj_t* root_{nullptr};

private:
    lv_obj_t* city_label_{nullptr};
    lv_obj_t* weather_icon_label_{nullptr};
    lv_obj_t* current_label_{nullptr};
    lv_obj_t* condition_label_{nullptr};
    lv_obj_t* feels_like_label_{nullptr};
    lv_obj_t* humidity_label_{nullptr};
    lv_obj_t* wind_label_{nullptr};
    lv_obj_t* forecast_day_labels_[5]{};
    lv_obj_t* forecast_icon_labels_[5]{};
    lv_obj_t* forecast_temp_labels_[5]{};
    lv_obj_t* updated_label_{nullptr};
    float refresh_accumulator_{0.0f};
};

} // namespace apps
