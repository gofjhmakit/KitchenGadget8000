#include "hal/Display.h"

#include <cstring>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"

namespace hal {
namespace {
constexpr const char* TAG = "Display";
constexpr size_t DRAW_BUF_LINES = 40;
constexpr ledc_mode_t BACKLIGHT_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t BACKLIGHT_TIMER = LEDC_TIMER_0;
constexpr ledc_channel_t BACKLIGHT_CHANNEL = LEDC_CHANNEL_0;

static esp_lcd_panel_handle_t to_panel(void* handle) {
    return static_cast<esp_lcd_panel_handle_t>(handle);
}
} // namespace

Display& Display::instance() {
    static Display inst;
    return inst;
}

void Display::flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* color_map) {
    auto* self = &Display::instance();
    esp_lcd_panel_handle_t panel = to_panel(self->panel_handle_);
    if (panel != nullptr) {
        esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    }
    lv_display_flush_ready(display);
}

bool Display::init() {
    if (lv_disp_ != nullptr) {
        return true;
    }

    gpio_config_t bk_gpio{};
    bk_gpio.pin_bit_mask = 1ULL << DisplayConfig::BACKLIGHT_GPIO;
    bk_gpio.mode = GPIO_MODE_OUTPUT;
    bk_gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;
    bk_gpio.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&bk_gpio);

    ledc_timer_config_t timer_cfg{};
    timer_cfg.speed_mode = BACKLIGHT_MODE;
    timer_cfg.timer_num = BACKLIGHT_TIMER;
    timer_cfg.duty_resolution = LEDC_TIMER_8_BIT;
    timer_cfg.freq_hz = 5000;
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg{};
    channel_cfg.gpio_num = DisplayConfig::BACKLIGHT_GPIO;
    channel_cfg.speed_mode = BACKLIGHT_MODE;
    channel_cfg.channel = BACKLIGHT_CHANNEL;
    channel_cfg.timer_sel = BACKLIGHT_TIMER;
    channel_cfg.duty = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));

    esp_lcd_rgb_panel_config_t panel_config{};
    panel_config.data_width = 16;
    panel_config.bits_per_pixel = 16;
    panel_config.num_fbs = 1;
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.pclk_gpio_num = DisplayConfig::PCLK_GPIO;
    panel_config.vsync_gpio_num = DisplayConfig::VSYNC_GPIO;
    panel_config.hsync_gpio_num = DisplayConfig::HSYNC_GPIO;
    panel_config.de_gpio_num = DisplayConfig::DE_GPIO;
    panel_config.disp_gpio_num = -1;
    int data_pins[16] = {
        DisplayConfig::B0_GPIO, DisplayConfig::B1_GPIO, DisplayConfig::B2_GPIO, DisplayConfig::B3_GPIO, DisplayConfig::B4_GPIO,
        DisplayConfig::G0_GPIO, DisplayConfig::G1_GPIO, DisplayConfig::G2_GPIO, DisplayConfig::G3_GPIO,
        DisplayConfig::G4_GPIO, DisplayConfig::G5_GPIO,
        DisplayConfig::R0_GPIO, DisplayConfig::R1_GPIO, DisplayConfig::R2_GPIO, DisplayConfig::R3_GPIO, DisplayConfig::R4_GPIO
    };
    std::memcpy(panel_config.data_gpio_nums, data_pins, sizeof(data_pins));
    panel_config.timings.pclk_hz = DisplayConfig::PCLK_HZ;
    panel_config.timings.h_res = DisplayConfig::WIDTH;
    panel_config.timings.v_res = DisplayConfig::HEIGHT;
    panel_config.timings.hsync_back_porch = 80;
    panel_config.timings.hsync_front_porch = 40;
    panel_config.timings.hsync_pulse_width = 48;
    panel_config.timings.vsync_back_porch = 23;
    panel_config.timings.vsync_front_porch = 13;
    panel_config.timings.vsync_pulse_width = 3;
    panel_config.flags.fb_in_psram = 1;

    esp_lcd_panel_handle_t panel = nullptr;
    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &panel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_rgb_panel failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    panel_handle_ = panel;

    const size_t buf_pixels = DisplayConfig::WIDTH * DRAW_BUF_LINES;
    auto* buf1 = static_cast<lv_color16_t*>(heap_caps_malloc(buf_pixels * sizeof(lv_color16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    auto* buf2 = static_cast<lv_color16_t*>(heap_caps_malloc(buf_pixels * sizeof(lv_color16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (buf1 == nullptr || buf2 == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate LVGL draw buffers");
        return false;
    }

    lv_disp_ = lv_display_create(DisplayConfig::WIDTH, DisplayConfig::HEIGHT);
    lv_display_set_color_format(lv_disp_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(lv_disp_, &Display::flush_cb);
    lv_display_set_buffers(lv_disp_, buf1, buf2, buf_pixels * sizeof(lv_color16_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(lv_disp_);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);
    set_backlight(200);
    return true;
}

void Display::set_backlight(uint8_t brightness) {
    ledc_set_duty(BACKLIGHT_MODE, BACKLIGHT_CHANNEL, brightness);
    ledc_update_duty(BACKLIGHT_MODE, BACKLIGHT_CHANNEL);
}

} // namespace hal
