#pragma once
#include "lvgl.h"
#include <cstdint>

namespace ui::anim {

void fade_in(lv_obj_t* obj, uint32_t duration = 250, uint32_t delay = 0);
void fade_out_and_delete(lv_obj_t* obj, uint32_t duration = 250);
void slide_in(lv_obj_t* obj, bool from_right = true, uint32_t duration = 280);
void slide_out_and_delete(lv_obj_t* obj, bool to_right = false, uint32_t duration = 240);
lv_obj_t* create_spinner(lv_obj_t* parent, int size = 48);
void pulse_glow(lv_obj_t* obj, uint32_t color = 0xFFD166, uint32_t duration = 900);
void stop_pulse(lv_obj_t* obj);

} // namespace ui::anim
