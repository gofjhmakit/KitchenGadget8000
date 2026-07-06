#include "core/Notifications.h"

#include "esp_timer.h"
#include "lvgl.h"
#include "ui/Theme.h"

namespace core {
namespace {
lv_color_t color_for(NotificationType type) {
    switch (type) {
        case NotificationType::SUCCESS: return lv_color_hex(ui::Color::SUCCESS);
        case NotificationType::WARNING: return lv_color_hex(ui::Color::WARNING);
        case NotificationType::ALARM: return lv_color_hex(ui::Color::ERROR);
        case NotificationType::INFO:
        default: return lv_color_hex(ui::Color::GOLD_HI);
    }
}

void toast_timer_cb(lv_timer_t* timer) {
    lv_obj_delete(static_cast<lv_obj_t*>(lv_timer_get_user_data(timer)));
    lv_timer_delete(timer);
}
}

Notifications& Notifications::instance() {
    static Notifications inst;
    return inst;
}

uint32_t Notifications::push(NotificationType type, const std::string& title, const std::string& message) {
    Notification notification{next_id_++, type, title, message, static_cast<uint32_t>(esp_timer_get_time() / 1000ULL), false};
    notifications_.push_back(notification);
    if (handler_) {
        handler_(notification);
    }

    lv_obj_t* screen = lv_screen_active();
    lv_obj_t* toast = lv_obj_create(screen);
    lv_obj_set_size(toast, 420, LV_SIZE_CONTENT);
    lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(toast, lv_color_hex(ui::Color::SURFACE_2), 0);
    lv_obj_set_style_border_color(toast, color_for(type), 0);
    lv_obj_set_style_border_width(toast, 2, 0);
    lv_obj_set_style_pad_all(toast, ui::Spacing::MD, 0);
    lv_obj_set_layout(toast, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(toast, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* title_lbl = lv_label_create(toast);
    lv_label_set_text(title_lbl, title.c_str());
    lv_obj_set_style_text_color(title_lbl, color_for(type), 0);
    lv_obj_set_style_text_font(title_lbl, ui::Theme::font_body(), 0);

    lv_obj_t* msg_lbl = lv_label_create(toast);
    lv_obj_set_width(msg_lbl, LV_PCT(100));
    lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(msg_lbl, message.c_str());
    lv_obj_set_style_text_color(msg_lbl, lv_color_hex(ui::Color::TEXT_PRI), 0);

    lv_timer_t* timer = lv_timer_create(toast_timer_cb, 3200, toast);
    lv_timer_set_repeat_count(timer, 1);
    return notification.id;
}

void Notifications::dismiss(uint32_t id) {
    for (auto& item : notifications_) {
        if (item.id == id) {
            item.dismissed = true;
            break;
        }
    }
}

void Notifications::dismiss_all() {
    for (auto& item : notifications_) {
        item.dismissed = true;
    }
}

} // namespace core
