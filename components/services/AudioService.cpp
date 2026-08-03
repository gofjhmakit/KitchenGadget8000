#include "services/AudioService.h"

#include <cmath>
#include <cstring>
#include "driver/gpio.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace services {
namespace {
constexpr const char* TAG = "AudioService";
constexpr gpio_num_t PIN_BCLK  = GPIO_NUM_22;
constexpr gpio_num_t PIN_LRCLK = GPIO_NUM_21;
constexpr gpio_num_t PIN_SDATA = GPIO_NUM_23;
constexpr gpio_num_t PIN_AMP   = GPIO_NUM_30;
constexpr uint32_t SAMPLE_RATE = 16000;
constexpr size_t DMA_FRAMES = 256;
constexpr float TWO_PI = 6.28318530717958647692f;

i2s_chan_handle_t tx_chan = nullptr;

void play_tone(float freq, uint32_t duration_ms, uint8_t volume) {
    if (!tx_chan || volume == 0) return;
    const float amplitude = static_cast<float>(0x7FFF) * static_cast<float>(volume) / 100.0f;
    const size_t total_samples = SAMPLE_RATE * duration_ms / 1000;
    constexpr size_t BUF_FRAMES = 256;
    static int16_t buf[BUF_FRAMES * 2];

    size_t written = 0;
    for (size_t offset = 0; offset < total_samples; offset += BUF_FRAMES) {
        const size_t frames = (total_samples - offset > BUF_FRAMES) ? BUF_FRAMES : (total_samples - offset);
        for (size_t i = 0; i < frames; ++i) {
            const float t = static_cast<float>(offset + i) / static_cast<float>(SAMPLE_RATE);
            const int16_t sample = static_cast<int16_t>(amplitude * sinf(TWO_PI * freq * t));
            buf[i * 2] = sample;
            buf[i * 2 + 1] = sample;
        }
        i2s_channel_write(tx_chan, buf, frames * 2 * sizeof(int16_t), &written, pdMS_TO_TICKS(100));
    }
}

void silence(uint32_t duration_ms) {
    if (!tx_chan) return;
    constexpr size_t BUF_FRAMES = 256;
    static int16_t buf[BUF_FRAMES * 2];
    std::memset(buf, 0, sizeof(buf));
    const size_t total_frames = SAMPLE_RATE * duration_ms / 1000;
    size_t written = 0;
    for (size_t offset = 0; offset < total_frames; offset += BUF_FRAMES) {
        const size_t frames = (total_frames - offset > BUF_FRAMES) ? BUF_FRAMES : (total_frames - offset);
        i2s_channel_write(tx_chan, buf, frames * 2 * sizeof(int16_t), &written, pdMS_TO_TICKS(100));
    }
}
} // namespace

AudioService& AudioService::instance() {
    static AudioService inst;
    return inst;
}

bool AudioService::init() {
    if (initialized_) return true;

    gpio_config_t amp_cfg{};
    amp_cfg.pin_bit_mask = 1ULL << static_cast<uint32_t>(PIN_AMP);
    amp_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_config(&amp_cfg);
    gpio_set_level(PIN_AMP, 1);  // HIGH = amp OFF (active-low NS4168)

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 6;
    chan_cfg.dma_frame_num = DMA_FRAMES;
    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_chan, nullptr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return false;
    }

    // Use exact slot config from Elecrow BSP: standard I2S (Philips), bit_shift=true
    i2s_std_slot_config_t slot = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    i2s_std_config_t std_cfg{
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = slot,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = PIN_BCLK,
            .ws   = PIN_LRCLK,
            .dout = PIN_SDATA,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };

    err = i2s_channel_init_std_mode(tx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(err));
        i2s_del_channel(tx_chan);
        tx_chan = nullptr;
        return false;
    }

    err = i2s_channel_enable(tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(err));
        i2s_del_channel(tx_chan);
        tx_chan = nullptr;
        return false;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "AudioService initialized");
    return true;
}

void AudioService::deinit() {
    stop_alarm();
    vTaskDelay(pdMS_TO_TICKS(200));
    if (tx_chan) {
        i2s_channel_disable(tx_chan);
        i2s_del_channel(tx_chan);
        tx_chan = nullptr;
    }
    gpio_set_level(PIN_AMP, 1);  // HIGH = amp OFF
    initialized_ = false;
}

struct AlarmTaskArg {
    AudioService* self;
    uint8_t type;
    uint8_t volume;
};

void AudioService::alarm_task(void* arg) {
    auto* a = static_cast<AlarmTaskArg*>(arg);
    a->self->generate_alarm(a->type, a->volume);
    delete a;
    vTaskDelete(nullptr);
}

void AudioService::generate_alarm(uint8_t type, uint8_t volume) {
    gpio_set_level(PIN_AMP, 0);   // LOW = amp ON (active-low NS4168)
    vTaskDelay(pdMS_TO_TICKS(120));  // NS4168 needs ~100ms to stabilise

    for (int rep = 0; rep < 5 && !stop_requested_; ++rep) {
        if (type == 0) {
            for (int i = 0; i < 3 && !stop_requested_; ++i) {
                play_tone(880.0f, 150, volume);
                if (!stop_requested_) silence(100);
            }
            if (!stop_requested_) silence(600);
        } else if (type == 1) {
            if (!stop_requested_) play_tone(523.25f, 200, volume);
            if (!stop_requested_) play_tone(659.25f, 200, volume);
            if (!stop_requested_) play_tone(783.99f, 200, volume);
            if (!stop_requested_) play_tone(1046.5f, 400, volume);
            if (!stop_requested_) silence(500);
        } else if (type == 2) {
            for (uint8_t v = volume; v > 5 && !stop_requested_; v = static_cast<uint8_t>(static_cast<float>(v) * 0.7f)) {
                play_tone(1000.0f, 80, v);
            }
            if (!stop_requested_) silence(800);
        }
    }

    silence(50);
    gpio_set_level(PIN_AMP, 1);  // HIGH = amp OFF
    playing_ = false;
    stop_requested_ = false;
}

void AudioService::play_alarm(uint8_t type, uint8_t volume) {
    if (!initialized_ && !init()) return;
    if (playing_) stop_alarm();
    if (type == 3) return;

    playing_ = true;
    stop_requested_ = false;
    auto* arg = new AlarmTaskArg{this, type, volume};
    xTaskCreate(alarm_task, "alarm", 8192, arg, 3, nullptr);
}

void AudioService::stop_alarm() {
    stop_requested_ = true;
    for (int i = 0; i < 100 && playing_; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

} // namespace services
