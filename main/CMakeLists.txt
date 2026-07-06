idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES hal ui core apps services nvs_flash esp_wifi esp_http_client esp_https_ota spiffs
)
