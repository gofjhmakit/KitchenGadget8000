#include <cstring>
#include <memory>
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "lvgl.h"

#include "hal/Display.h"
#include "hal/Touch.h"
#include "ui/Theme.h"
#include "core/AppManager.h"
#include "core/BottomNav.h"
#include "core/Navigation.h"
#include "core/Storage.h"
#include "core/Settings.h"
#include "core/Network.h"
#include "core/Notifications.h"
#include "core/PowerManager.h"
#include "services/WiFiService.h"
#include "services/AudioService.h"
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

// ── LVGL SPIFFS filesystem driver ────────────────────────────────────────────
// Drive letter 'S' — LVGL image paths use the form "S:/spiffs/recipes/foo.jpg"
// which maps to the POSIX path /spiffs/recipes/foo.jpg on the VFS.
static lv_fs_drv_t spiffs_lv_drv;

static void* lvgl_spiffs_open(lv_fs_drv_t*, const char* path, lv_fs_mode_t mode) {
    return fopen(path, (mode == LV_FS_MODE_WR) ? "wb" : "rb");
}
static lv_fs_res_t lvgl_spiffs_close(lv_fs_drv_t*, void* fp) {
    fclose(static_cast<FILE*>(fp)); return LV_FS_RES_OK;
}
static lv_fs_res_t lvgl_spiffs_read(lv_fs_drv_t*, void* fp, void* buf, uint32_t btr, uint32_t* br) {
    *br = fread(buf, 1, btr, static_cast<FILE*>(fp)); return LV_FS_RES_OK;
}
static lv_fs_res_t lvgl_spiffs_seek(lv_fs_drv_t*, void* fp, uint32_t pos, lv_fs_whence_t whence) {
    int w = (whence == LV_FS_SEEK_SET) ? SEEK_SET : (whence == LV_FS_SEEK_CUR) ? SEEK_CUR : SEEK_END;
    fseek(static_cast<FILE*>(fp), static_cast<long>(pos), w); return LV_FS_RES_OK;
}
static lv_fs_res_t lvgl_spiffs_tell(lv_fs_drv_t*, void* fp, uint32_t* pos) {
    *pos = static_cast<uint32_t>(ftell(static_cast<FILE*>(fp))); return LV_FS_RES_OK;
}

static void register_lvgl_spiffs_driver() {
    lv_fs_drv_init(&spiffs_lv_drv);
    spiffs_lv_drv.letter   = 'S';
    spiffs_lv_drv.open_cb  = lvgl_spiffs_open;
    spiffs_lv_drv.close_cb = lvgl_spiffs_close;
    spiffs_lv_drv.read_cb  = lvgl_spiffs_read;
    spiffs_lv_drv.seek_cb  = lvgl_spiffs_seek;
    spiffs_lv_drv.tell_cb  = lvgl_spiffs_tell;
    lv_fs_drv_register(&spiffs_lv_drv);
}
// ─────────────────────────────────────────────────────────────────────────────

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
    // ── Early visual heartbeat: blink backlight 3× so we can confirm the app started
    // This runs before ANY other init — if the screen backlight blinks, the app is alive.
    {
        gpio_config_t bk{};
        bk.pin_bit_mask = 1ULL << hal::DisplayConfig::BACKLIGHT_GPIO;
        bk.mode = GPIO_MODE_OUTPUT;
        gpio_config(&bk);
        for (int i = 0; i < 3; i++) {
            gpio_set_level(static_cast<gpio_num_t>(hal::DisplayConfig::BACKLIGHT_GPIO), 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(static_cast<gpio_num_t>(hal::DisplayConfig::BACKLIGHT_GPIO), 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "KitchenGadget8000 starting... (boot)");
    ESP_LOGI(TAG, "========================================");

    ESP_LOGI(TAG, "[1/14] Initialising NVS flash...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS: erasing and re-initialising");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_LOGI(TAG, "[1/14] NVS OK");

    ESP_LOGI(TAG, "[2/14] Loading settings...");
    core::Settings::instance().init();
    core::Settings::instance().load();
    const auto& cfg = core::Settings::instance().get();
    ESP_LOGI(TAG, "[2/14] Settings OK");

    ESP_LOGI(TAG, "[3/14] Mounting SPIFFS storage...");
    core::Storage::instance().init("/spiffs");
    ESP_LOGI(TAG, "[3/14] Storage OK");

    ESP_LOGI(TAG, "[4/14] Initialising LVGL...");
    lv_init();
    register_lvgl_spiffs_driver();
    ESP_LOGI(TAG, "[4/14] LVGL OK");

    ESP_LOGI(TAG, "[5/14] Initialising display (MIPI DSI + EK79007 + backlight)...");
    if (!hal::Display::instance().init()) {
        ESP_LOGE(TAG, "[5/14] Display init FAILED — halting");
        return;
    }
    ESP_LOGI(TAG, "[5/14] Display OK");

    ESP_LOGI(TAG, "[6/14] Initialising touch controller (GT911)...");
    if (!hal::Touch::instance().init()) {
        ESP_LOGW(TAG, "[6/14] Touch init failed; continuing without touch");
    } else {
        ESP_LOGI(TAG, "[6/14] Touch OK");
    }

    ESP_LOGI(TAG, "[7/14] Applying UI theme...");
    ui::Theme::instance().init();
    ui::Theme::instance().apply();
    ESP_LOGI(TAG, "[7/14] Theme OK");

    ESP_LOGI(TAG, "[8/14] Creating LVGL mutex...");
    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == nullptr) {
        ESP_LOGE(TAG, "[8/14] Failed to create LVGL mutex — halting");
        return;
    }
    ESP_LOGI(TAG, "[8/14] LVGL mutex OK");

    ESP_LOGI(TAG, "[9/14] Initialising power manager...");
    core::PowerManager::instance().init();
    core::PowerManager::instance().set_backlight(cfg.backlight);
    core::PowerManager::instance().set_screensaver_timeout(cfg.screensaver_timeout);
    ESP_LOGI(TAG, "[9/14] Power manager OK (backlight=%d)", cfg.backlight);

    ESP_LOGI(TAG, "[10/14] Initialising network...");
    core::Network::instance().init();
    ESP_LOGI(TAG, "[10/14] Network OK");

    ESP_LOGI(TAG, "[11/14] Registering apps...");
    auto& mgr = core::AppManager::instance();
    mgr.set_root_screen(lv_screen_active());
    core::BottomNav::instance().init(lv_screen_active());
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
    ESP_LOGI(TAG, "[11/14] Apps registered");

    ESP_LOGI(TAG, "[12/14] Setting up power state callback and launching launcher...");
    core::PowerManager::instance().set_state_callback([&](core::PowerState state) {
        if (state == core::PowerState::SCREENSAVER) {
            core::Navigation::instance().navigate_to(core::AppId::SCREENSAVER, core::AppManager::Transition::FADE);
        } else if (state == core::PowerState::ACTIVE && core::AppManager::instance().current_app() == core::AppId::SCREENSAVER) {
            core::Navigation::instance().go_home();
        }
    });

    mgr.launch(core::AppId::LAUNCHER);
    ESP_LOGI(TAG, "[12/14] Launcher launched");

    ESP_LOGI(TAG, "[13/14] Starting LVGL tasks...");
    xTaskCreatePinnedToCore(lvgl_tick_task, "lvgl_tick", 2048, nullptr, 5, nullptr, 0);
    xTaskCreatePinnedToCore(lvgl_handler_task, "lvgl_handler", 8192, nullptr, 4, nullptr, 1);
    xTaskCreatePinnedToCore(app_update_task, "app_update", 4096, nullptr, 3, nullptr, 1);
    ESP_LOGI(TAG, "[13/14] LVGL tasks started");

    ESP_LOGI(TAG, "[14/14] Starting background services...");
    const bool audio_ok = services::AudioService::instance().init();
    ESP_LOGI(TAG, "[14/14] AudioService: %s", audio_ok ? "OK" : "FAILED (no alarm sound)");
    if (std::strlen(cfg.wifi_ssid) > 0) {
        ESP_LOGI(TAG, "[14/14] Starting WiFi (SSID: %s)...", cfg.wifi_ssid);
        services::WiFiService::instance().start(cfg.wifi_ssid, cfg.wifi_password);
    } else {
        ESP_LOGW(TAG, "[14/14] No WiFi SSID configured — skipping WiFi");
    }
    services::TimeService::instance().init();
    services::GitHubSync::instance().init(cfg.github_repo, cfg.github_branch);
    ESP_LOGI(TAG, "[14/14] Background services started");

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "KitchenGadget8000 running.");
    ESP_LOGI(TAG, "========================================");
}
