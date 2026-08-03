idf_component_register(
    SRCS "WiFiService.cpp" "GitHubSync.cpp" "OTAService.cpp"
          "TimeService.cpp" "MarkdownParser.cpp" "AudioService.cpp"
    INCLUDE_DIRS "include" "${PROJECT_DIR}/main"
    REQUIRES lvgl core esp_wifi esp_http_client esp_https_ota lwip esp_event esp_netif mdns app_update driver esp_driver_i2s
)
