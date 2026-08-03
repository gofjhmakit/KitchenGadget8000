cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(KitchenGadget8000)

if(CONFIG_ESP_WIFI_REMOTE_ENABLED AND NOT CONFIG_ESP_WIFI_ENABLED)
    # On ESP32-P4 the on-board Wi-Fi is provided by esp_wifi_remote (via esp_hosted
    # talking to the ESP32-C6 co-processor).  CONFIG_ESP_WIFI_ENABLED is gated on
    # SOC_WIFI_SUPPORTED, which is off for the P4, but the esp_wifi.h APIs are
    # still available through esp_wifi_remote.  Define the macro so our
    # CONFIG_ESP_WIFI_ENABLED-guarded application code compiles in.
    add_compile_definitions(CONFIG_ESP_WIFI_ENABLED=1)
endif()
