#include "ui/Animations.h"

#include "ui/Theme.h"

namespace ui::anim {
namespace {
void set_x(void* obj, int32_t v) { lv_obj_set_x(static_cast<lv_obj_t*>(obj), v); }
void set_opa(void* obj, int32_t v) { lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(v), 0); }
void set_shadow(void* obj, int32_t v) { lv_obj_set_style_shadow_width(static_cast<lv_obj_t*>(obj), v, 0); }
void set_arc(void* obj, int32_t v) { lv_arc_set_value(static_cast<lv_obj_t*>(obj), v); }
void delete_cb(lv_anim_t* a) { lv_obj_delete(static_cast<lv_obj_t*>(a->var)); }
}

void fade_in(lv_obj_t* obj, uint32_t duration, uint32_t delay) {
    lv_obj_set_style_opa(obj, LV_OPA_0, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_exec_cb(&a, set_opa);
    lv_anim_start(&a);
}

void fade_out_and_delete(lv_obj_t* obj, uint32_t duration) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_0);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_exec_cb(&a, set_opa);
    lv_anim_set_completed_cb(&a, delete_cb);
    lv_anim_start(&a);
}

void slide_in(lv_obj_t* obj, bool from_right, uint32_t duration) {
    lv_coord_t width = lv_obj_get_width(lv_obj_get_parent(obj));
    lv_coord_t start = from_right ? width : -width;
    lv_obj_set_x(obj, start);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, start, 0);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, set_x);
    lv_anim_start(&a);
}

void slide_out_and_delete(lv_obj_t* obj, bool to_right, uint32_t duration) {
    lv_coord_t width = lv_obj_get_width(lv_obj_get_parent(obj));
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, lv_obj_get_x(obj), to_right ? width : -width);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_exec_cb(&a, set_x);
    lv_anim_set_completed_cb(&a, delete_cb);
    lv_anim_start(&a);
}

lv_obj_t* create_spinner(lv_obj_t* parent, int size) {
    lv_obj_t* spinner = lv_arc_create(parent);
    lv_obj_set_size(spinner, size, size);
    lv_arc_set_rotation(spinner, 270);
    lv_arc_set_bg_angles(spinner, 0, 360);
    lv_arc_set_range(spinner, 0, 100);
    lv_arc_set_value(spinner, 25);
    lv_obj_remove_style(spinner, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(Color::GOLD_TRACK), LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(Color::GOLD_HI), LV_PART_INDICATOR);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, spinner);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, 900);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, set_arc);
    lv_anim_start(&a);
    return spinner;
}

void pulse_glow(lv_obj_t* obj, uint32_t color, uint32_t duration) {
    lv_obj_set_style_shadow_color(obj, lv_color_hex(color), 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 4, 28);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_playback_duration(&a, duration);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, set_shadow);
    lv_anim_start(&a);
}

void stop_pulse(lv_obj_t* obj) {
    lv_anim_delete(obj, set_shadow);
    lv_obj_set_style_shadow_width(obj, 0, 0);
}

void animate_arc_to(lv_obj_t* arc, int32_t to_value, uint32_t duration) {
    lv_anim_delete(arc, set_arc);
    const int32_t from_value = lv_arc_get_value(arc);
    if (from_value == to_value) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_values(&a, from_value, to_value);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, set_arc);
    lv_anim_start(&a);
}

void blink(lv_obj_t* obj, uint32_t half_period_ms) {
    lv_anim_delete(obj, set_opa);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_20);
    lv_anim_set_duration(&a, half_period_ms);
    lv_anim_set_playback_duration(&a, half_period_ms);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, set_opa);
    lv_anim_start(&a);
}

void stop_blink(lv_obj_t* obj) {
    lv_anim_delete(obj, set_opa);
    lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
}

} // namespace ui::anim
