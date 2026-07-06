#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace core {

enum class NotificationType {
    INFO,
    SUCCESS,
    WARNING,
    ALARM
};

struct Notification {
    uint32_t id;
    NotificationType type;
    std::string title;
    std::string message;
    uint32_t timestamp_ms;
    bool dismissed{false};
};

class Notifications {
public:
    static Notifications& instance();
    uint32_t push(NotificationType type, const std::string& title, const std::string& message);
    void dismiss(uint32_t id);
    void dismiss_all();
    const std::vector<Notification>& all() const { return notifications_; }
    using Handler = std::function<void(const Notification&)>;
    void set_handler(Handler h) { handler_ = h; }

private:
    Notifications() = default;
    std::vector<Notification> notifications_;
    Handler handler_;
    uint32_t next_id_{1};
};

} // namespace core
