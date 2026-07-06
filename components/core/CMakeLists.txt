idf_component_register(
    SRCS "AppManager.cpp" "Navigation.cpp" "Storage.cpp"
          "Settings.cpp" "Network.cpp" "Notifications.cpp" "PowerManager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES lvgl spiffs nvs_flash esp_wifi esp_http_client ui driver esp_event esp_netif esp_timer
)
