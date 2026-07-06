#include "core/Navigation.h"

namespace core {

Navigation& Navigation::instance() {
    static Navigation inst;
    return inst;
}

void Navigation::navigate_to(AppId id, AppManager::Transition t) {
    AppId current_id = AppManager::instance().current_app();
    if (current_id != id) {
        history_.push(current_id);
    }
    AppManager::instance().set_transition(t);
    AppManager::instance().launch(id);
}

void Navigation::go_back() {
    if (history_.empty()) {
        go_home();
        return;
    }
    const AppId target = history_.top();
    history_.pop();
    AppManager::instance().set_transition(AppManager::Transition::SLIDE_RIGHT);
    AppManager::instance().launch(target);
}

void Navigation::go_home() {
    while (!history_.empty()) {
        history_.pop();
    }
    AppManager::instance().set_transition(AppManager::Transition::FADE);
    AppManager::instance().launch(AppId::LAUNCHER);
}

AppId Navigation::current() const {
    return AppManager::instance().current_app();
}

bool Navigation::can_go_back() const {
    return !history_.empty();
}

} // namespace core
