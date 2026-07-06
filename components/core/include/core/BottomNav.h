#pragma once
#include "lvgl.h"
#include "AppManager.h"

namespace core {

class BottomNav {
public:
    static constexpr lv_coord_t HEIGHT = 58;

    static BottomNav& instance();

    void init(lv_obj_t* root_screen);
    void set_active(AppId id);
    void show();
    void hide();
    void reposition();   ///< Recalculate pos/size from current display dims (call after rotation)
    bool is_visible() const { return visible_; }
    lv_obj_t* obj() const { return container_; }

private:
    BottomNav() = default;

    struct NavItem {
        AppId   id;
        const char* icon;
        const char* label;
    };
    static constexpr NavItem kItems[5] = {
        {AppId::LAUNCHER,      LV_SYMBOL_HOME,     "Home"},
        {AppId::TIMERS,        LV_SYMBOL_BELL,     "Timers"},
        {AppId::LIGHTING,      LV_SYMBOL_TINT,     "Light"},
        {AppId::SHOPPING_LIST, LV_SYMBOL_CHARGE,   "Shop"},
        {AppId::WEATHER,       LV_SYMBOL_SETTINGS, "More"},
    };

    lv_obj_t* container_{nullptr};
    lv_obj_t* buttons_[5]{};
    lv_obj_t* icon_labels_[5]{};
    lv_obj_t* text_labels_[5]{};
    bool visible_{false};
};

} // namespace core
