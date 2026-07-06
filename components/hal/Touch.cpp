#include "hal/Touch.h"

#include "driver/i2c_master.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"

namespace hal {
namespace {
constexpr const char* TAG = "Touch";
}

Touch& Touch::instance() {
    static Touch inst;
    return inst;
}

bool Touch::init() {
    if (lv_indev_ != nullptr) {
        return true;
    }

    i2c_master_bus_config_t bus_cfg{};
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.i2c_port = static_cast<i2c_port_num_t>(TouchConfig::I2C_PORT);
    bus_cfg.sda_io_num = static_cast<gpio_num_t>(TouchConfig::SDA_GPIO);
    bus_cfg.scl_io_num = static_cast<gpio_num_t>(TouchConfig::SCL_GPIO);
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle = nullptr;
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.dev_addr = TouchConfig::GT911_ADDR;
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    err = esp_lcd_new_panel_io_i2c(bus_handle, &io_cfg, &io_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_i2c failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_lcd_touch_config_t touch_cfg{};
    touch_cfg.x_max = 1024;
    touch_cfg.y_max = 600;
    touch_cfg.rst_gpio_num = static_cast<gpio_num_t>(TouchConfig::RST_GPIO);
    touch_cfg.int_gpio_num = static_cast<gpio_num_t>(TouchConfig::INT_GPIO);
    touch_cfg.levels.reset = 0;
    touch_cfg.levels.interrupt = 0;
    touch_cfg.flags.swap_xy = 0;
    touch_cfg.flags.mirror_x = 0;
    touch_cfg.flags.mirror_y = 0;

    esp_lcd_touch_handle_t tp = nullptr;
    err = esp_lcd_touch_new_i2c_gt911(io_handle, &touch_cfg, &tp);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_touch_new_i2c_gt911 failed: %s", esp_err_to_name(err));
        return false;
    }
    i2c_dev_ = tp;

    lv_indev_ = lv_indev_create();
    lv_indev_set_type(lv_indev_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lv_indev_, &Touch::read_cb);
    return true;
}

void Touch::read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* self = &Touch::instance();
    auto touch = static_cast<esp_lcd_touch_handle_t>(self->i2c_dev_);
    if (touch == nullptr) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    esp_lcd_touch_read_data(touch);
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t strength = 0;
    uint8_t count = 0;
    bool pressed = esp_lcd_touch_get_coordinates(touch, &x, &y, &strength, &count, 1);
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = x;
    data->point.y = y;
    (void)indev;
}

} // namespace hal
