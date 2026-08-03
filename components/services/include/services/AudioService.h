#pragma once
#include <cstdint>

namespace services {

// I2S1 audio on Elecrow CrowPanel Advanced 9" ESP32-P4
// BCLK=GPIO22, LRCLK=GPIO21, SDATA=GPIO23, AMP_EN=GPIO30 (active-high)
class AudioService {
public:
    static AudioService& instance();

    bool init();
    void deinit();

    void play_alarm(uint8_t type, uint8_t volume);
    void stop_alarm();

    bool is_playing() const { return playing_; }

private:
    AudioService() = default;
    bool initialized_{false};
    volatile bool playing_{false};
    volatile bool stop_requested_{false};

    static void alarm_task(void* arg);
    void generate_alarm(uint8_t type, uint8_t volume);
};

} // namespace services
