#include "services/TimeService.h"

#include <ctime>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"

namespace services {

TimeService& TimeService::instance() {
    static TimeService inst;
    return inst;
}

void TimeService::init() {
    if (initialized_) return;
    set_timezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, const_cast<char*>("pool.ntp.org"));
    esp_sntp_init();
    initialized_ = true;
}

void TimeService::set_timezone(const char* tz) {
    setenv("TZ", tz, 1);
    tzset();
}

std::tm TimeService::now_local() const {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_r(&now, &tm);
    return tm;
}

std::string TimeService::time_string(bool with_seconds) const {
    char buf[16];
    auto tm = now_local();
    std::strftime(buf, sizeof(buf), with_seconds ? "%H:%M:%S" : "%H:%M", &tm);
    return buf;
}

std::string TimeService::date_string() const {
    char buf[32];
    auto tm = now_local();
    std::strftime(buf, sizeof(buf), "%d %b %Y", &tm);
    return buf;
}

std::string TimeService::day_name() const {
    char buf[16];
    auto tm = now_local();
    std::strftime(buf, sizeof(buf), "%A", &tm);
    return buf;
}

std::string TimeService::timestamp_string() const {
    return date_string() + "  " + time_string(true);
}

} // namespace services
