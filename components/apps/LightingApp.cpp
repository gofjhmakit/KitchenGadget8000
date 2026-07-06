#include "apps/LightingApp.h"

#include <cstring>
#include <cstdio>
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
void update_state(lv_event_t* e) {
    auto* app = static_cast<LightingApp*>(lv_event_get_user_data(e));
    const bool on = lv_obj_has_state(app->power_switch_, LV_STATE_CHECKED);
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s • %d%% • %dK", on ? "On" : "Off", lv_slider_get_value(app->brightness_slider_), 2200 + lv_slider_get_value(app->temp_slider_) * 40);
    lv_label_set_text(app->scene_label_, buf);
}
}

void LightingApp::on_mount(lv_obj_t* parent) { root_ = parent; build_ui(parent); }

void LightingApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont, ui::Spacing::LG, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(cont, "Lighting");
    power_switch_ = lv_switch_create(cont);
    brightness_slider_ = lv_slider_create(cont); lv_slider_set_range(brightness_slider_, 0, 100); lv_slider_set_value(brightness_slider_, 80, LV_ANIM_OFF);
    temp_slider_ = lv_slider_create(cont); lv_slider_set_range(temp_slider_, 0, 100); lv_slider_set_value(temp_slider_, 40, LV_ANIM_OFF);
    scene_label_ = lv_label_create(cont);
    for (auto* obj : {power_switch_, brightness_slider_, temp_slider_}) lv_obj_add_event_cb(obj, update_state, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_t* scenes = lv_obj_create(cont);
    lv_obj_remove_style_all(scenes);
    lv_obj_set_layout(scenes, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scenes, LV_FLEX_FLOW_ROW);
    for (const char* scene : {"Cooking", "Dining", "Ambient", "Off"}) {
        lv_obj_t* btn = ui::create_gold_button(scenes, scene);
        lv_obj_add_event_cb(btn, [](lv_event_t* e){ static_cast<LightingApp*>(lv_event_get_user_data(e))->apply_scene(lv_label_get_text(lv_obj_get_child(lv_event_get_target_obj(e), 0))); }, LV_EVENT_CLICKED, this);
    }
    update_state(nullptr);
}

void LightingApp::apply_scene(const char* scene) {
    if (std::strcmp(scene, "Cooking") == 0) { lv_obj_add_state(power_switch_, LV_STATE_CHECKED); lv_slider_set_value(brightness_slider_, 100, LV_ANIM_OFF); lv_slider_set_value(temp_slider_, 70, LV_ANIM_OFF); }
    else if (std::strcmp(scene, "Dining") == 0) { lv_obj_add_state(power_switch_, LV_STATE_CHECKED); lv_slider_set_value(brightness_slider_, 55, LV_ANIM_OFF); lv_slider_set_value(temp_slider_, 35, LV_ANIM_OFF); }
    else if (std::strcmp(scene, "Ambient") == 0) { lv_obj_add_state(power_switch_, LV_STATE_CHECKED); lv_slider_set_value(brightness_slider_, 28, LV_ANIM_OFF); lv_slider_set_value(temp_slider_, 18, LV_ANIM_OFF); }
    else { lv_obj_clear_state(power_switch_, LV_STATE_CHECKED); lv_slider_set_value(brightness_slider_, 0, LV_ANIM_OFF); }
    char buf[96];
    std::snprintf(buf, sizeof(buf), "Scene: %s", scene);
    lv_label_set_text(scene_label_, buf);
}

void LightingApp::on_unmount() { root_ = nullptr; }
void LightingApp::on_update(float) {}

} // namespace apps
