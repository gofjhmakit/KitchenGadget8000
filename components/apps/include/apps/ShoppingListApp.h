#pragma once
#include "core/AppManager.h"
#include "lvgl.h"
#include <string>
#include <vector>

namespace apps {

struct ShoppingItem { std::string text; std::string category; bool done{false}; };

class ShoppingListApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::SHOPPING_LIST; }
    const char* name() const override { return "Shopping"; }
    const char* icon() const override { return "🛒"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    void load();
    void save();
    void rebuild_list();
    lv_obj_t* root_{nullptr};
    lv_obj_t* list_{nullptr};
    lv_obj_t* item_ta_{nullptr};
    lv_obj_t* category_dd_{nullptr};
    std::vector<ShoppingItem> items_{};
};

} // namespace apps
