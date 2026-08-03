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

    struct NavItem {
        AppId   id;
        const char* icon;
        const char* label;
    };
    static constexpr NavItem kItems[5] = {
        {AppId::LAUNCHER,  LV_SYMBOL_HOME,     "Home"},
        {AppId::TIMERS,    LV_SYMBOL_BELL,     "Timers"},
        {AppId::NOTES,     LV_SYMBOL_EDIT,     "Notes"},
        {AppId::WEATHER,   LV_SYMBOL_GPS,      "Weather"},
        {AppId::LAUNCHER,  LV_SYMBOL_BARS,     "Apps"},  // index 4 = All Apps view
    };

private:
    BottomNav() = default;

    lv_obj_t* container_{nullptr};
    lv_obj_t* buttons_[5]{};
    lv_obj_t* icon_labels_[5]{};
    lv_obj_t* text_labels_[5]{};
    bool visible_{false};
};

} // namespace core
