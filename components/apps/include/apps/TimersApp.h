#pragma once
#include "core/AppManager.h"
#include "lvgl.h"
#include <array>
#include <cstdint>

namespace apps {

struct KitchenTimer {
    static constexpr uint8_t SLOT_COUNT = 5;
    uint32_t duration_sec{0};
    uint32_t remaining_sec{0};
    bool active{false};
    bool expired{false};
    bool running{false};
    char name[32]{"Timer"};
    uint32_t color{0xFFD166};
    char emoji[8]{"⏱"};
};

class TimersApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::TIMERS; }
    const char* name() const override { return "Timers"; }
    const char* icon() const override { return "⏱"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
    bool has_active_timers() const;
    void add_timer(uint32_t duration_sec, const char* name = "Timer", const char* emoji = "⏱");
public:
    void build_ui(lv_obj_t* parent);
    void update_timer_display(int slot);
    void handle_timer_expired(int slot);
    void show_add_timer_dialog(lv_obj_t* parent);
    std::array<KitchenTimer, KitchenTimer::SLOT_COUNT> timers_{};
    lv_obj_t* root_{nullptr};
    lv_obj_t* timer_cards_[KitchenTimer::SLOT_COUNT]{};
    lv_obj_t* arcs_[KitchenTimer::SLOT_COUNT]{};
    lv_obj_t* time_labels_[KitchenTimer::SLOT_COUNT]{};
    lv_obj_t* name_labels_[KitchenTimer::SLOT_COUNT]{};
    lv_obj_t* play_btns_[KitchenTimer::SLOT_COUNT]{};
    lv_obj_t* empty_label_{nullptr};
    int active_slot_{0};
    float second_accumulator_{0.0f};
};

} // namespace apps
