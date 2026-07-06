#include "core/AppManager.h"

#include <algorithm>
#include "ui/Animations.h"
#include "ui/Theme.h"

namespace core {

AppManager& AppManager::instance() {
    static AppManager inst;
    return inst;
}

void AppManager::register_app(std::unique_ptr<IApp> app) {
    auto exists = std::find_if(apps_.begin(), apps_.end(), [&](const auto& item) { return item->id() == app->id(); });
    if (exists == apps_.end()) {
        apps_.push_back(std::move(app));
    }
}

IApp* AppManager::find(AppId id) const {
    auto it = std::find_if(apps_.begin(), apps_.end(), [&](const auto& app) { return app->id() == id; });
    return it == apps_.end() ? nullptr : it->get();
}

void AppManager::launch(AppId id) {
    if (root_screen_ == nullptr) {
        root_screen_ = lv_screen_active();
    }
    IApp* next = find(id);
    if (next == nullptr) {
        return;
    }

    previous_id_ = current_id_;
    current_id_ = id;

    lv_obj_t* old_container = current_container_;
    if (current_ != nullptr) {
        current_->on_unmount();
    }

    current_container_ = lv_obj_create(root_screen_);
    lv_obj_remove_style_all(current_container_);
    lv_obj_set_size(current_container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(current_container_, lv_color_hex(ui::Color::BG), 0);
    lv_obj_set_style_bg_opa(current_container_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(current_container_, 0, 0);
    current_ = next;
    current_->on_mount(current_container_);

    switch (pending_transition_) {
        case Transition::SLIDE_LEFT:
            ui::anim::slide_in(current_container_, true);
            if (old_container != nullptr) ui::anim::slide_out_and_delete(old_container, false);
            break;
        case Transition::SLIDE_RIGHT:
            ui::anim::slide_in(current_container_, false);
            if (old_container != nullptr) ui::anim::slide_out_and_delete(old_container, true);
            break;
        case Transition::FADE:
            ui::anim::fade_in(current_container_);
            if (old_container != nullptr) ui::anim::fade_out_and_delete(old_container);
            break;
        case Transition::ZOOM_IN:
        case Transition::NONE:
        default:
            if (old_container != nullptr) {
                lv_obj_delete(old_container);
            }
            break;
    }
    pending_transition_ = Transition::FADE;
}

void AppManager::back() {
    launch(previous_id_);
}

AppId AppManager::current_app() const { return current_id_; }
AppId AppManager::previous_app() const { return previous_id_; }

void AppManager::update(float delta_sec) {
    if (current_ != nullptr) {
        current_->on_update(delta_sec);
    }
}

} // namespace core
