#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "lvgl.h"

namespace core {

enum class AppId : uint8_t {
    LAUNCHER = 0,
    TIMERS,
    CONVERTER,
    INGREDIENT_SCALER,
    BREAD_HYDRATION,
    DRINK_CHILLER,
    MEAT_TEMPERATURES,
    DISHWASHER,
    WEATHER,
    RECIPES,
    LIGHTING,
    SHOPPING_LIST,
    NOTES,
    SCREENSAVER,
    COUNT
};

class IApp {
public:
    virtual ~IApp() = default;
    virtual void on_mount(lv_obj_t* parent) = 0;
    virtual void on_unmount() = 0;
    virtual void on_update(float delta_sec) = 0;
    virtual AppId id() const = 0;
    virtual const char* name() const = 0;
    virtual const char* icon() const = 0;
};

class AppManager {
public:
    enum class Transition { NONE, SLIDE_LEFT, SLIDE_RIGHT, FADE, ZOOM_IN };

    static AppManager& instance();
    void register_app(std::unique_ptr<IApp> app);
    void launch(AppId id);
    void back();
    AppId current_app() const;
    AppId previous_app() const;
    void update(float delta_sec);
    lv_obj_t* root_screen() const { return root_screen_; }
    void set_root_screen(lv_obj_t* screen) { root_screen_ = screen; }
    void set_transition(Transition t) { pending_transition_ = t; }
    IApp* app(AppId id) const { return find(id); }

private:
    AppManager() = default;
    IApp* find(AppId id) const;

    std::vector<std::unique_ptr<IApp>> apps_;
    IApp* current_{nullptr};
    AppId current_id_{AppId::LAUNCHER};
    AppId previous_id_{AppId::LAUNCHER};
    lv_obj_t* root_screen_{nullptr};
    lv_obj_t* current_container_{nullptr};
    Transition pending_transition_{Transition::FADE};
};

} // namespace core
