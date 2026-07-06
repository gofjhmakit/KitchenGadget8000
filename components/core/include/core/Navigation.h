#pragma once
#include <stack>
#include "AppManager.h"

namespace core {

class Navigation {
public:
    static Navigation& instance();
    void navigate_to(AppId id, AppManager::Transition t = AppManager::Transition::SLIDE_LEFT);
    void go_back();
    void go_home();
    AppId current() const;
    bool can_go_back() const;

private:
    Navigation() = default;
    std::stack<AppId> history_;
};

} // namespace core
