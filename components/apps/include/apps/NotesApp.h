#pragma once
#include "core/AppManager.h"
#include "lvgl.h"
#include <string>
#include <vector>

namespace apps {

struct NoteItem { std::string title; std::string body; };

class NotesApp : public core::IApp {
public:
    core::AppId id() const override { return core::AppId::NOTES; }
    const char* name() const override { return "Notes"; }
    const char* icon() const override { return "✎"; }
    void on_mount(lv_obj_t* parent) override;
    void on_unmount() override;
    void on_update(float delta_sec) override;
public:
    void build_ui(lv_obj_t* parent);
    void load();
    void save();
    void rebuild_list();
    void open_editor(int index);
    lv_obj_t* root_{nullptr};
    lv_obj_t* list_{nullptr};
    lv_obj_t* editor_{nullptr};
    lv_obj_t* title_ta_{nullptr};
    lv_obj_t* body_ta_{nullptr};
    std::vector<NoteItem> notes_{};
    int editing_index_{-1};
};

} // namespace apps
