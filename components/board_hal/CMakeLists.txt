idf_component_register(
    SRCS "Display.cpp" "Touch.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd esp_lcd_touch esp_lcd_touch_gt911 driver esp_timer lvgl esp_psram
)
