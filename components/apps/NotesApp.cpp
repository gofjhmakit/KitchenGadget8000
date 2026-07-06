#include "apps/NotesApp.h"

#include <sstream>
#include "core/Storage.h"
#include "ui/Theme.h"
#include "ui/Widgets.h"

namespace apps {
namespace {
constexpr const char* PATH = "/notes.txt";
}

void NotesApp::on_mount(lv_obj_t* parent) { root_ = parent; load(); build_ui(parent); rebuild_list(); }

void NotesApp::build_ui(lv_obj_t* parent) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(cont, ui::Spacing::LG, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    ui::create_section_title(cont, "Notes");
    lv_obj_t* add = ui::create_gold_button(cont, "New note");
    lv_obj_add_event_cb(add, [](lv_event_t* e){ static_cast<NotesApp*>(lv_event_get_user_data(e))->open_editor(-1); }, LV_EVENT_CLICKED, this);
    list_ = lv_obj_create(cont);
    lv_obj_set_size(list_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(list_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_, LV_FLEX_FLOW_COLUMN);
}

void NotesApp::load() {
    notes_.clear();
    std::string data;
    if (!core::Storage::instance().read_file(PATH, data)) return;
    std::istringstream ss(data);
    std::string line;
    while (std::getline(ss, line, '\x1e')) {
        if (line.empty()) continue;
        auto sep = line.find('\x1f');
        if (sep != std::string::npos) notes_.push_back({line.substr(0, sep), line.substr(sep + 1)});
    }
}

void NotesApp::save() {
    std::ostringstream ss;
    for (const auto& note : notes_) ss << note.title << '\x1f' << note.body << '\x1e';
    core::Storage::instance().write_file(PATH, ss.str());
}

void NotesApp::rebuild_list() {
    if (!list_) return;
    lv_obj_clean(list_);
    for (size_t i = 0; i < notes_.size(); ++i) {
        const auto& note = notes_[i];
        std::string preview = note.body.substr(0, std::min<size_t>(note.body.size(), 80));
        lv_obj_t* item = ui::create_list_item(list_, note.title.c_str(), preview.c_str(), "📝");
        lv_obj_set_user_data(item, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(item, [](lv_event_t* e){ auto* app = static_cast<NotesApp*>(lv_event_get_user_data(e)); app->open_editor(static_cast<int>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))))); }, LV_EVENT_CLICKED, this);
    }
}

void NotesApp::open_editor(int index) {
    editing_index_ = index;
    if (editor_) lv_obj_delete(editor_);
    editor_ = ui::create_card(root_);
    lv_obj_set_size(editor_, 700, 520);
    lv_obj_center(editor_);
    lv_obj_set_layout(editor_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(editor_, LV_FLEX_FLOW_COLUMN);
    title_ta_ = lv_textarea_create(editor_); lv_obj_set_width(title_ta_, LV_PCT(100)); lv_textarea_set_one_line(title_ta_, true);
    body_ta_ = lv_textarea_create(editor_); lv_obj_set_width(body_ta_, LV_PCT(100)); lv_obj_set_height(body_ta_, 280);
    if (index >= 0 && index < static_cast<int>(notes_.size())) {
        lv_textarea_set_text(title_ta_, notes_[index].title.c_str());
        lv_textarea_set_text(body_ta_, notes_[index].body.c_str());
    }
    lv_obj_t* actions = lv_obj_create(editor_); lv_obj_remove_style_all(actions); lv_obj_set_layout(actions, LV_LAYOUT_FLEX); lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_t* save_btn = ui::create_gold_button(actions, "Save");
    lv_obj_add_event_cb(save_btn, [](lv_event_t* e){ auto* app = static_cast<NotesApp*>(lv_event_get_user_data(e)); NoteItem note{lv_textarea_get_text(app->title_ta_), lv_textarea_get_text(app->body_ta_)}; if (app->editing_index_ >= 0 && app->editing_index_ < static_cast<int>(app->notes_.size())) app->notes_[app->editing_index_] = note; else app->notes_.push_back(note); app->save(); app->rebuild_list(); lv_obj_delete(app->editor_); app->editor_ = nullptr; }, LV_EVENT_CLICKED, this);
    if (index >= 0) {
        lv_obj_t* del_btn = ui::create_gold_button(actions, "Delete");
        lv_obj_add_event_cb(del_btn, [](lv_event_t* e){ auto* app = static_cast<NotesApp*>(lv_event_get_user_data(e)); if (app->editing_index_ >= 0 && app->editing_index_ < static_cast<int>(app->notes_.size())) app->notes_.erase(app->notes_.begin() + app->editing_index_); app->save(); app->rebuild_list(); lv_obj_delete(app->editor_); app->editor_ = nullptr; }, LV_EVENT_CLICKED, this);
    }
    lv_obj_t* close_btn = ui::create_gold_button(actions, "Close");
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e){ auto* app = static_cast<NotesApp*>(lv_event_get_user_data(e)); lv_obj_delete(app->editor_); app->editor_ = nullptr; }, LV_EVENT_CLICKED, this);
}

void NotesApp::on_unmount() { root_ = nullptr; }
void NotesApp::on_update(float) {}

} // namespace apps
