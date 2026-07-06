#include <cstring>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lvgl.h"

#include "hal/Display.h"
#include "hal/Touch.h"
#include "ui/Theme.h"
#include "core/AppManager.h"
#include "core/Navigation.h"
#include "core/Storage.h"
#include "core/Settings.h"
#include "core/Network.h"
#include "core/Notifications.h"
#include "core/PowerManager.h"
#include "services/WiFiService.h"
#include "services/TimeService.h"
#include "services/GitHubSync.h"
#include "apps/LauncherApp.h"
#include "apps/ScreensaverApp.h"
#include "apps/TimersApp.h"
#include "apps/ConverterApp.h"
#include "apps/IngredientScalerApp.h"
#include "apps/BreadHydrationApp.h"
#include "apps/DrinkChillerApp.h"
#include "apps/MeatTemperaturesApp.h"
#include "apps/DishwasherApp.h"
#include "apps/WeatherApp.h"
#include "apps/RecipesApp.h"
#include "apps/LightingApp.h"
#include "apps/ShoppingListApp.h"
#include "apps/NotesApp.h"

static const char* TAG = "KG8000";
static SemaphoreHandle_t lvgl_mutex = nullptr;

static void lvgl_tick_task(void* arg) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5));
        lv_tick_inc(5);
    }
}

static void lvgl_handler_task(void* arg) {
    while (true) {
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(lvgl_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void app_update_task(void* arg) {
    TickType_t last_wake = xTaskGetTickCount();
    constexpr TickType_t period = pdMS_TO_TICKS(16);
    while (true) {
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            constexpr float delta = 0.016f;
            core::AppManager::instance().update(delta);
            core::PowerManager::instance().update(delta);
            xSemaphoreGive(lvgl_mutex);
        }
        vTaskDelayUntil(&last_wake, period);
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "KitchenGadget8000 starting...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    core::Settings::instance().init();
    core::Settings::instance().load();
    const auto& cfg = core::Settings::instance().get();

    core::Storage::instance().init("/spiffs");
    lv_init();

    if (!hal::Display::instance().init()) {
        ESP_LOGE(TAG, "Display init failed");
        return;
    }
    if (!hal::Touch::instance().init()) {
        ESP_LOGW(TAG, "Touch init failed; continuing without touch");
    }

    ui::Theme::instance().init();
    ui::Theme::instance().apply();

    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == nullptr) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        return;
    }

    core::PowerManager::instance().init();
    core::PowerManager::instance().set_backlight(cfg.backlight);
    core::PowerManager::instance().set_screensaver_timeout(cfg.screensaver_timeout);
    core::Network::instance().init();

    auto& mgr = core::AppManager::instance();
    mgr.set_root_screen(lv_screen_active());
    mgr.register_app(std::make_unique<apps::LauncherApp>());
    mgr.register_app(std::make_unique<apps::ScreensaverApp>());
    mgr.register_app(std::make_unique<apps::TimersApp>());
    mgr.register_app(std::make_unique<apps::ConverterApp>());
    mgr.register_app(std::make_unique<apps::IngredientScalerApp>());
    mgr.register_app(std::make_unique<apps::BreadHydrationApp>());
    mgr.register_app(std::make_unique<apps::DrinkChillerApp>());
    mgr.register_app(std::make_unique<apps::MeatTemperaturesApp>());
    mgr.register_app(std::make_unique<apps::DishwasherApp>());
    mgr.register_app(std::make_unique<apps::WeatherApp>());
    mgr.register_app(std::make_unique<apps::RecipesApp>());
    mgr.register_app(std::make_unique<apps::LightingApp>());
    mgr.register_app(std::make_unique<apps::ShoppingListApp>());
    mgr.register_app(std::make_unique<apps::NotesApp>());

    core::PowerManager::instance().set_state_callback([&](core::PowerState state) {
        if (state == core::PowerState::SCREENSAVER) {
            core::Navigation::instance().navigate_to(core::AppId::SCREENSAVER, core::AppManager::Transition::FADE);
        } else if (state == core::PowerState::ACTIVE && core::AppManager::instance().current_app() == core::AppId::SCREENSAVER) {
            core::Navigation::instance().go_home();
        }
    });

    mgr.launch(core::AppId::LAUNCHER);

    xTaskCreatePinnedToCore(lvgl_tick_task, "lvgl_tick", 2048, nullptr, 5, nullptr, 0);
    xTaskCreatePinnedToCore(lvgl_handler_task, "lvgl_handler", 8192, nullptr, 4, nullptr, 1);
    xTaskCreatePinnedToCore(app_update_task, "app_update", 4096, nullptr, 3, nullptr, 1);

    if (std::strlen(cfg.wifi_ssid) > 0) {
        services::WiFiService::instance().start(cfg.wifi_ssid, cfg.wifi_password);
    }
    services::TimeService::instance().init();
    services::GitHubSync::instance().init(cfg.github_repo, cfg.github_branch);

    ESP_LOGI(TAG, "KitchenGadget8000 running.");
}
