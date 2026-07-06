#pragma once
#include <string>
#include <ctime>

namespace services {

class TimeService {
public:
    static TimeService& instance();
    void init();
    void set_timezone(const char* tz);
    std::string time_string(bool with_seconds = false) const;
    std::string date_string() const;
    std::string day_name() const;
    std::string timestamp_string() const;
    std::tm now_local() const;

public:
    TimeService() = default;
    bool initialized_{false};
};

} // namespace services
