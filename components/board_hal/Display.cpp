#include "hal/Display.h"

#include <cstring>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_ek79007.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"

namespace hal {
namespace {
constexpr const char* TAG = "Display";
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

// use_dma2d=false → draw_bitmap is synchronous (CPU memcpy to PSRAM frame buffer).
// This is simpler and avoids the async DMA race with the LVGL render buffer.
// The 40-line partial buffer is 80KB; CPU copies this in ~80µs, fast enough for smooth UI.
void Display::flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* color_map) {
    esp_lcd_panel_handle_t panel = to_panel(Display::instance().panel_handle_);
    if (panel != nullptr) {
        esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    }
    lv_display_flush_ready(display);
}

bool Display::init() {
    if (lv_disp_ != nullptr) {
        return true;
    }

    // Backlight on GPIO31, 11-bit resolution, 30 kHz, PLL_DIV clock (per Elecrow BSP)
    gpio_config_t bk_gpio{};
    bk_gpio.pin_bit_mask = 1ULL << DisplayConfig::BACKLIGHT_GPIO;
    bk_gpio.mode = GPIO_MODE_OUTPUT;
    bk_gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;
    bk_gpio.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&bk_gpio);

    ledc_timer_config_t timer_cfg{};
    timer_cfg.speed_mode = BACKLIGHT_MODE;
    timer_cfg.timer_num = BACKLIGHT_TIMER;
    timer_cfg.duty_resolution = LEDC_TIMER_11_BIT;
    timer_cfg.freq_hz = 30000;
    timer_cfg.clk_cfg = LEDC_USE_PLL_DIV_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg{};
    channel_cfg.gpio_num = DisplayConfig::BACKLIGHT_GPIO;
    channel_cfg.speed_mode = BACKLIGHT_MODE;
    channel_cfg.channel = BACKLIGHT_CHANNEL;
    channel_cfg.timer_sel = BACKLIGHT_TIMER;
    channel_cfg.duty = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));

    // Power on MIPI DSI PHY via internal LDO (channel 3 → VDD_MIPI_DPHY, 2.5V)
    esp_ldo_channel_handle_t ldo_mipi_phy = nullptr;
    esp_ldo_channel_config_t ldo_config{};
    ldo_config.chan_id = 3;
    ldo_config.voltage_mv = 2500;
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_config, &ldo_mipi_phy));
    ESP_LOGI(TAG, "MIPI DSI PHY LDO enabled");

    // MIPI DSI bus: 2 data lanes at 900 Mbps
    esp_lcd_dsi_bus_handle_t dsi_bus = nullptr;
    esp_lcd_dsi_bus_config_t bus_config{};
    bus_config.bus_id = 0;
    bus_config.num_data_lanes = 2;
    bus_config.phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT;
    bus_config.lane_bit_rate_mbps = 900;
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &dsi_bus));
    dsi_bus_ = dsi_bus;

    // DBI I/O for EK79007 init commands
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_dbi_io_config_t dbi_config{};
    dbi_config.virtual_channel = 0;
    dbi_config.lcd_cmd_bits = 8;
    dbi_config.lcd_param_bits = 8;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &io_handle));
    io_handle_ = io_handle;

    // DPI video timing for 1024×600
    esp_lcd_dpi_panel_config_t dpi_config{};
    dpi_config.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
    dpi_config.dpi_clock_freq_mhz = 52;   // per EK79007_1024_600_PANEL_60HZ_CONFIG macro
    dpi_config.virtual_channel = 0;
    dpi_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
    dpi_config.num_fbs = 1;
    dpi_config.video_timing.h_size = DisplayConfig::WIDTH;
    dpi_config.video_timing.v_size = DisplayConfig::HEIGHT;
    dpi_config.video_timing.hsync_back_porch = 160;
    dpi_config.video_timing.hsync_pulse_width = 10;  // was 70, driver reference is 10
    dpi_config.video_timing.hsync_front_porch = 160;
    dpi_config.video_timing.vsync_back_porch = 23;
    dpi_config.video_timing.vsync_pulse_width = 1;   // was 10, driver reference is 1
    dpi_config.video_timing.vsync_front_porch = 12;
    dpi_config.flags.use_dma2d = false;  // CPU copy: synchronous, no async race with LVGL buffer

    ek79007_vendor_config_t vendor_config{};
    vendor_config.mipi_config.dsi_bus = dsi_bus;
    vendor_config.mipi_config.dpi_config = &dpi_config;

    esp_lcd_panel_dev_config_t panel_config{};
    panel_config.reset_gpio_num = -1;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel = 16;
    panel_config.vendor_config = &vendor_config;

    esp_lcd_panel_handle_t panel = nullptr;
    esp_err_t err = esp_lcd_new_panel_ek79007(io_handle, &panel_config, &panel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_ek79007 failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    panel_handle_ = panel;

    // Partial SRAM buffer: 1024×40 lines = 80KB
    // Fast for rendering; CPU copy to PSRAM frame buffer takes ~80µs per flush
    constexpr size_t LINES = 40;
    constexpr size_t buf_bytes = DisplayConfig::WIDTH * LINES * sizeof(lv_color16_t);
    auto* buf1 = static_cast<lv_color16_t*>(heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    auto* buf2 = static_cast<lv_color16_t*>(heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (buf1 == nullptr || buf2 == nullptr) {
        ESP_LOGW(TAG, "Internal RAM full, falling back to PSRAM for draw buffer");
        buf1 = static_cast<lv_color16_t*>(heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        buf2 = static_cast<lv_color16_t*>(heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
    if (buf1 == nullptr || buf2 == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate LVGL draw buffers");
        return false;
    }

    lv_disp_ = lv_display_create(DisplayConfig::WIDTH, DisplayConfig::HEIGHT);
    lv_display_set_color_format(lv_disp_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(lv_disp_, &Display::flush_cb);
    lv_display_set_buffers(lv_disp_, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(lv_disp_);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);
    set_backlight(200);
    return true;
}

void Display::set_backlight(uint8_t brightness) {
    uint32_t duty = (brightness == 0) ? 0 : ((uint32_t)brightness * 7 + 200);
    ledc_set_duty(BACKLIGHT_MODE, BACKLIGHT_CHANNEL, duty);
    ledc_update_duty(BACKLIGHT_MODE, BACKLIGHT_CHANNEL);
}

} // namespace hal
