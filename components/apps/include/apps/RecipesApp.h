#pragma once
#include "core/AppManager.h"
#include "lvgl.h"
#include <string>
#include <vector>
#include "services/MarkdownParser.h"

namespace apps {

class RecipesApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::RECIPES; }
    const char* name() const override { return "Recipes"; }
    const char* icon() const override { return "📖"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;

    void build_ui(lv_obj_t* parent);
    void load_recipes();
    void show_list();
    void show_tag_bar();
    void show_detail(size_t index);

    /// Fullscreen: hide BottomNav, expand container to full display height.
    void enter_fullscreen();
    void exit_fullscreen();

    /// Portrait/landscape rotation. Portrait forces fullscreen.
    /// Switching back to landscape also exits fullscreen.
    void toggle_orientation();

    lv_obj_t* root_{nullptr};
    lv_obj_t* content_{nullptr};
    lv_obj_t* header_{nullptr};
    lv_obj_t* header_title_{nullptr};
    lv_obj_t* back_btn_{nullptr};

    bool step_mode_{true};
    bool fullscreen_{false};
    bool portrait_{false};
    size_t current_index_{0};
    size_t step_index_{0};
    std::string active_tag_{};
    std::vector<services::Recipe> recipes_{};
    std::vector<std::string> all_tags_{};
    lv_obj_t* tag_bar_{nullptr};
    lv_obj_t* count_label_{nullptr};
};

} // namespace apps
