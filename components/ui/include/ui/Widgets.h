#pragma once
#include <string>
#include "lvgl.h"

namespace ui {

lv_obj_t* create_progress_ring(lv_obj_t* parent, uint32_t color, int size = 180);
lv_obj_t* create_stat_card(lv_obj_t* parent, const char* title, const char* value,
                           const char* subtitle = nullptr, const char* icon = nullptr);
lv_obj_t* create_list_item(lv_obj_t* parent, const char* title,
                           const char* subtitle = nullptr, const char* icon = nullptr);
lv_obj_t* create_numpad(lv_obj_t* parent, lv_obj_t* target_textarea,
                        bool allow_decimal = true, bool allow_negative = false);

// ── New design-guide helpers ─────────────────────────────────────────────────

// Two-line app header: TITLE (gold, all-caps) on the left; optional action
// button on the right.  Returns the header container object.
lv_obj_t* create_app_header(lv_obj_t* parent, const char* title,
                             const char* action_label = nullptr,
                             lv_event_cb_t action_cb = nullptr,
                             void* action_user_data = nullptr);

// Recipe image: loads a JPEG from SPIFFS via LVGL FS driver when the file
// exists; falls back to a styled card with emoji_fallback otherwise.
lv_obj_t* create_recipe_image(lv_obj_t* parent, const std::string& spiffs_path,
                               const char* emoji_fallback,
                               lv_coord_t w, lv_coord_t h);

// Modal numpad overlay.  parent should be the full screen object.
// On "Done", copies the entered value into target_ta and fires
// LV_EVENT_VALUE_CHANGED on it, then closes the overlay.
void show_numpad_modal(lv_obj_t* screen, lv_obj_t* target_ta,
                       bool allow_decimal = true, bool allow_negative = false);

// Styled LVGL keyboard.  parent should be the full screen object.
// The keyboard closes itself when the user presses OK or Cancel.
lv_obj_t* show_styled_keyboard(lv_obj_t* screen, lv_obj_t* textarea,
                                lv_keyboard_mode_t mode = LV_KEYBOARD_MODE_TEXT_LOWER);

} // namespace ui
