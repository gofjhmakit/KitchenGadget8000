#pragma once
#include "lvgl.h"

namespace ui {

lv_obj_t* create_progress_ring(lv_obj_t* parent, uint32_t color, int size = 180);
lv_obj_t* create_stat_card(lv_obj_t* parent, const char* title, const char* value,
                           const char* subtitle = nullptr, const char* icon = nullptr);
lv_obj_t* create_list_item(lv_obj_t* parent, const char* title,
                           const char* subtitle = nullptr, const char* icon = nullptr);
lv_obj_t* create_numpad(lv_obj_t* parent, lv_obj_t* target_textarea,
                        bool allow_decimal = true, bool allow_negative = false);

} // namespace ui
