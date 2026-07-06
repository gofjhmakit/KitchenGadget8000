#include "core/BottomNav.h"

#include "core/Navigation.h"
#include "core/Notifications.h"
#include "ui/Theme.h"

namespace core {
namespace {

void nav_click(lv_event_t* e) {
    const auto* item = static_cast<const BottomNav::NavItem*>(lv_event_get_user_data(e));
    // "More" slot re-mapped to weather; still navigates normally
    Navigation::instance().navigate_to(item->id, AppManager::Transition::FADE);
}

} // namespace

// NavItem constexpr definition (required in exactly one TU for older C++ linkers)
constexpr BottomNav::NavItem BottomNav::kItems[5];

BottomNav& BottomNav::instance() {
    static BottomNav inst;
    return inst;
}

void BottomNav::init(lv_obj_t* root_screen) {
    if (container_ != nullptr) return;

    const lv_coord_t display_w = lv_display_get_horizontal_resolution(lv_display_get_default());
    const lv_coord_t display_h = lv_display_get_vertical_resolution(lv_display_get_default());

    container_ = lv_obj_create(root_screen);
    lv_obj_remove_style_all(container_);
    lv_obj_set_size(container_, display_w, HEIGHT);
    lv_obj_set_pos(container_, 0, display_h - HEIGHT);
    lv_obj_set_style_bg_color(container_, lv_color_hex(ui::Color::SURFACE), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    // Thin gold top border
    lv_obj_set_style_border_side(container_, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(container_, lv_color_hex(ui::Color::GOLD_DIM), 0);
    lv_obj_set_style_border_width(container_, 1, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_layout(container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < 5; ++i) {
        buttons_[i] = lv_obj_create(container_);
        lv_obj_remove_style_all(buttons_[i]);
        lv_obj_set_size(buttons_[i], LV_PCT(20), HEIGHT);
        lv_obj_set_style_bg_opa(buttons_[i], LV_OPA_0, 0);
        lv_obj_set_style_bg_color(buttons_[i], lv_color_hex(ui::Color::GOLD_FAINT), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(buttons_[i], LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_add_flag(buttons_[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_radius(buttons_[i], 0, 0);
        lv_obj_set_layout(buttons_[i], LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(buttons_[i], LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(buttons_[i], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(buttons_[i], 2, 0);

        icon_labels_[i] = lv_label_create(buttons_[i]);
        lv_label_set_text(icon_labels_[i], kItems[i].icon);
        lv_obj_set_style_text_font(icon_labels_[i], ui::Theme::font_heading(), 0);
        lv_obj_set_style_text_color(icon_labels_[i], lv_color_hex(ui::Color::TEXT_HINT), 0);

        text_labels_[i] = lv_label_create(buttons_[i]);
        lv_label_set_text(text_labels_[i], kItems[i].label);
        lv_obj_set_style_text_font(text_labels_[i], ui::Theme::font_small(), 0);
        lv_obj_set_style_text_color(text_labels_[i], lv_color_hex(ui::Color::TEXT_HINT), 0);

        lv_obj_add_event_cb(buttons_[i], nav_click, LV_EVENT_CLICKED,
                            const_cast<NavItem*>(&kItems[i]));
    }

    visible_ = true;
    lv_obj_move_foreground(container_);
}

void BottomNav::set_active(AppId id) {
    if (container_ == nullptr) return;
    for (int i = 0; i < 5; ++i) {
        // Only the first matching slot becomes active (avoids lighting both Home+More)
        const bool active = (kItems[i].id == id) && (i == 0 || kItems[i - 1].id != id);
        const uint32_t icon_color = active ? ui::Color::GOLD_HI : ui::Color::TEXT_HINT;
        const uint32_t text_color = active ? ui::Color::GOLD    : ui::Color::TEXT_HINT;
        lv_obj_set_style_text_color(icon_labels_[i], lv_color_hex(icon_color), 0);
        lv_obj_set_style_text_color(text_labels_[i], lv_color_hex(text_color), 0);
        // Gold underline bar for active item
        lv_obj_set_style_border_side(buttons_[i], active ? LV_BORDER_SIDE_TOP : LV_BORDER_SIDE_NONE, 0);
        lv_obj_set_style_border_color(buttons_[i], lv_color_hex(ui::Color::GOLD_HI), 0);
        lv_obj_set_style_border_width(buttons_[i], active ? 2 : 0, 0);
    }
}

void BottomNav::show() {
    if (container_ == nullptr) return;
    lv_obj_remove_flag(container_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(container_);
    visible_ = true;
}

void BottomNav::hide() {
    if (container_ == nullptr) return;
    lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
    visible_ = false;
}

} // namespace core
