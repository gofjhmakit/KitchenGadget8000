#pragma once
#include <functional>
#include <string>

namespace services {

class OTAService {
public:
    using ProgressCallback = std::function<void(int)>;
    static OTAService& instance();
    bool check_for_update(const std::string& repo, std::string& version, std::string& url);
    bool apply_update(const std::string& url, ProgressCallback progress_cb = {});

public:
    OTAService() = default;
};

} // namespace services
