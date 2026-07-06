#pragma once
#include <string>

namespace services {

class GitHubSync {
public:
    static GitHubSync& instance();
    void init(const char* repo, const char* branch);
    void set_interval(uint32_t seconds);
    bool sync_once();

public:
    GitHubSync() = default;
    std::string repo_;
    std::string branch_;
    uint32_t interval_sec_{3600};
    bool initialized_{false};
};

} // namespace services
