idf_component_register(
    SRCS "WiFiService.cpp" "GitHubSync.cpp" "OTAService.cpp"
          "TimeService.cpp" "MarkdownParser.cpp"
    INCLUDE_DIRS "include"
    REQUIRES lvgl core esp_wifi esp_http_client esp_https_ota lwip esp_event esp_netif mdns app_update
)
