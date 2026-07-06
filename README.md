# KitchenGadget8000

**Version 2 of KitchenGadget6000** — a premium, purpose-built kitchen assistant running on the Elecrow CrowPanel Advanced 9" ESP32-P4 HMI display.

> Think Bang & Olufsen meets kitchen appliance. Not an Android tablet. Not a maker project. A dedicated, polished consumer-grade kitchen tool.

---

## Overview

KitchenGadget8000 is an embedded kitchen assistant with a luxury **black and gold** aesthetic. It runs on the ESP32-P4 with a 1024×600 IPS display and provides a rich suite of kitchen tools — all available offline. WiFi enables optional cloud features like weather, dishwasher monitoring, and automatic recipe synchronisation from GitHub.

Everything from [KitchenGadget6000](https://github.com/gofjhmakit/KitchenGadget6000) is present and extended.

---

## Hardware

**Target device:** [Elecrow CrowPanel Advanced 9" ESP32-P4 HMI AI Display](https://www.elecrow.com/crowpanel-advanced-9inch-esp32-p4-hmi-ai-display-1024x600-ips-touch-screen-wifi-6-support.html)

| Component | Details |
|-----------|---------|
| MCU | ESP32-P4 (dual-core RISC-V, up to 400 MHz) |
| Connectivity | ESP32-C6 co-processor (WiFi 6 + BLE 5) |
| Display | 9" IPS, 1024×600, RGB parallel interface |
| Touch | Capacitive GT911, I2C |
| Power | USB-C charging dock + internal LiPo (PH2.0-2P, 3.7 V, ~430 mA charging) |
| Storage | Internal flash + SPIFFS for recipes and user data |

---

## Software Architecture

The codebase is strictly modular. `main.cpp` is a thin entry point; all logic lives in components.

```
KitchenGadget8000/
├── main/                   Entry point (main.cpp), Kconfig
├── components/
│   ├── hal/                Hardware Abstraction Layer
│   │   ├── Display         RGB LCD panel init, LVGL flush, backlight PWM
│   │   └── Touch           GT911 I2C touch, LVGL input device
│   ├── ui/                 UI framework
│   │   ├── Theme           Gold/black colour palette, font accessors
│   │   ├── Widgets         Reusable LVGL widgets (cards, buttons, numpad, …)
│   │   └── Animations      Screen transitions, pulse glow, fade, slide
│   ├── core/               Core application subsystems
│   │   ├── AppManager      App lifecycle, screen transitions
│   │   ├── Navigation      Back-stack navigation
│   │   ├── Storage         SPIFFS file I/O
│   │   ├── Settings        NVS-backed application settings
│   │   ├── Network         WiFi + HTTP client wrapper
│   │   ├── Notifications   Toast/alarm notification queue
│   │   └── PowerManager    Idle timeout, screensaver trigger, battery ADC, backlight
│   ├── apps/               Individual app modules (one class per app)
│   │   ├── LauncherApp     Home screen — app grid, clock, battery, WiFi status
│   │   ├── ScreensaverApp  Clock screensaver with ambient animation
│   │   ├── TimersApp       Up to 5 concurrent countdown timers
│   │   ├── ConverterApp    Volume / weight / temperature unit conversion
│   │   ├── IngredientScalerApp  Scale recipe ingredient amounts
│   │   ├── BreadHydrationApp    Baker's hydration calculator
│   │   ├── DrinkChillerApp      Drink chilling time estimator
│   │   ├── MeatTemperaturesApp  Safe internal temperature reference
│   │   ├── DishwasherApp        Bosch HomeConnect IoT integration
│   │   ├── WeatherApp           Open-Meteo weather (current + tomorrow)
│   │   ├── RecipesApp           Markdown recipe browser with timer integration
│   │   ├── LightingApp          Abstract lighting control (IKEA Tradfri-ready)
│   │   ├── ShoppingListApp      Persistent categorised shopping list
│   │   └── NotesApp             Persistent freeform notes
│   └── services/           Background services
│       ├── WiFiService     Auto-reconnect WiFi wrapper
│       ├── GitHubSync      Recipe sync from GitHub (SHA-based delta updates)
│       ├── OTAService      Firmware OTA from GitHub Releases
│       ├── TimeService     SNTP time sync, formatted time/date strings
│       └── MarkdownParser  Frontmatter + recipe Markdown parser
└── recipes/                50 healthy recipe Markdown files
```

### Key design principles

- **No global variables** — all state lives in singleton services or app instances.
- **Dependency injection** where practical; singletons with `::instance()` for global services.
- **Separation of concerns** — UI in `apps/`, business logic in `core/` and `services/`.
- **Offline-first** — all kitchen tools work without WiFi; cloud features degrade gracefully.
- **LVGL v9 API** throughout.
- **Modern C++17** — `constexpr`, scoped enums, `std::unique_ptr`, `std::array`.

---

## Features

### Apps

| App | Description |
|-----|-------------|
| **Launcher** | 4×3 app grid, live clock, battery indicator, WiFi status |
| **Screensaver** | Full-screen clock with ambient gold pulsing animation; activates after 2 min idle (blocked when timers are running or recipe is open) |
| **Timers** | 5 simultaneous colour-coded countdown timers, arc rings, quick presets (5/10/15/30/60 min), custom numpad entry, alarm notification |
| **Converter** | Volume (ml/L/tsp/tbsp/fl oz/cup/pint/quart/gallon), Weight (g/kg/oz/lb), Temperature (°C/°F/K) |
| **Ingredient Scaler** | Enter amount, original servings, target servings — get scaled quantity |
| **Bread Hydration** | Enter flour weight + target hydration % → water weight with reference table |
| **Drink Chiller** | Newton's cooling law estimator: choose container, start/target temperature, location (freezer/fridge) |
| **Meat Temperatures** | Quick-reference safe internal temperatures (°C and °F) for beef, chicken, pork, fish, lamb, ground meat |
| **Dishwasher** | Bosch HomeConnect REST polling: status, program, remaining time arc, finished notification |
| **Weather** | Open-Meteo: current temperature, condition, today H/L, tomorrow forecast; no API key required |
| **Recipes** | Browse/search 50+ Markdown recipes; ingredient checklist; one-step-at-a-time mode; tappable timer shortcuts from step text |
| **Lighting** | Abstract on/off, brightness, colour temperature, preset scenes (Cooking/Dining/Ambient); IKEA Tradfri plug-in ready |
| **Shopping List** | Add/check/delete items, categorised, SPIFFS-persisted |
| **Notes** | Freeform notes, SPIFFS-persisted |

### Services

| Service | Description |
|---------|-------------|
| **WiFiService** | Auto-reconnect, non-blocking startup |
| **GitHubSync** | Checks `recipes/` folder on GitHub periodically; downloads only changed files using SHA ETags |
| **OTAService** | Firmware updates from GitHub Releases |
| **TimeService** | SNTP, timezone, formatted strings |
| **MarkdownParser** | YAML frontmatter + Markdown sections (Ingredients, Instructions, Notes), embedded timer extraction |

### Power Management

- Battery voltage ADC + percentage display
- Charging state detection
- Backlight PWM control
- Screensaver activates after configurable idle timeout (default 2 min)
- Screensaver blocked when timers are running or recipe detail is open

---

## Recipe Format

Recipes are Markdown files in `/recipes/`. On-device recipes live in SPIFFS at `/spiffs/recipes/`. GitHub Sync downloads updated files automatically.

```markdown
---
title: Herb Roasted Salmon with Asparagus
time: 25 min
servings: 4
tags: [main, fish, healthy]
image: 🐟
---

## Ingredients
- 4 salmon fillets
- 400 g asparagus
- 2 tbsp olive oil

## Instructions
1. Place salmon and asparagus on a tray.
2. Brush with herb oil mixture.
3. Roast at 200°C for 14 minutes.

## Notes
Great with boiled potatoes.
```

Timer shortcuts are detected automatically from instruction text (e.g. "14 minutes", "1 hour").

---

## Building

### Prerequisites

- [ESP-IDF v5.3+](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/get-started/index.html)
- CMake ≥ 3.16
- Python 3.8+ (for IDF tools)
- LVGL managed component (fetched automatically via `idf_component_manager`)

### Credentials

Copy the example file and fill in your values:

```bash
cp secrets.h.example main/secrets.h
```

`main/secrets.h` is in `.gitignore` and will never be committed.

```cpp
#define WIFI_SSID        "your_network"
#define WIFI_PASSWORD    "your_password"
#define HC_ACCESS_TOKEN  "your_home_connect_token"  // Bosch dishwasher
#define HC_APPLIANCE_ID  ""                          // leave empty to auto-discover
#define GITHUB_TOKEN     ""                          // optional, for higher rate limits
```

### Build & Flash

```bash
# Set up IDF environment
. $IDF_PATH/export.sh

# Configure target
idf.py set-target esp32p4

# (Optional) open menuconfig to adjust settings
idf.py menuconfig

# Build
idf.py build

# Flash and monitor (replace /dev/ttyUSB0 with your port)
idf.py -p /dev/ttyUSB0 flash monitor
```

### SPIFFS: Uploading Recipes

Recipe files need to be flashed to the SPIFFS partition. Create a SPIFFS image and flash it:

```bash
# Install spiffsgen.py tool (part of ESP-IDF)
python $IDF_PATH/components/spiffs/spiffsgen.py \
    10092544 recipes/ spiffs.bin

# Flash to the spiffs partition offset
esptool.py -p /dev/ttyUSB0 write_flash 0x620000 spiffs.bin
```

Or copy the recipes directory to `main/spiffs_image/` and configure the `SPIFFS_IMAGE` component in CMake to auto-generate the image.

---

## Configuration (menuconfig)

| Key | Default | Description |
|-----|---------|-------------|
| `KG8000_WEATHER_LAT` | 62.1302 | Latitude for weather data |
| `KG8000_WEATHER_LON` | 25.6728 | Longitude for weather data |
| `KG8000_SCREENSAVER_TIMEOUT_SEC` | 120 | Idle seconds before screensaver |

Settings can also be changed at runtime via the Settings app (future feature) and are persisted in NVS.

---

## Design Language

| Element | Value |
|---------|-------|
| Background | `#000000` pure black |
| Gold (bright) | `#FFD166` headings, active elements, large numbers |
| Gold (mid) | `#C9A84C` labels, arc indicators |
| Gold (dim) | `#7A6025` secondary hints |
| Text primary | `#FFFFFF` |
| Text secondary | `#B0A080` |
| Cards | `#0D0D0D` background, `#1E1A10` border |

Typography uses LVGL's built-in Montserrat font family (14–48 pt).

---

## Extending

### Adding a new app

1. Create `components/apps/MyApp.h` / `MyApp.cpp` inheriting `core::IApp`.
2. Add a new `AppId` entry in `core/AppManager.h`.
3. Register the app in `main/main.cpp`: `mgr.register_app(std::make_unique<apps::MyApp>());`
4. Add an icon entry to `LauncherApp.cpp` `kApps` array.
5. Add `MyApp.cpp` to `components/apps/CMakeLists.txt`.

### Adding a lighting backend (e.g. IKEA Tradfri)

Implement `apps::ILightingBackend` (declared in `LightingApp.h`) and inject it:
```cpp
LightingApp::instance().set_backend(std::make_unique<TradfriBackend>());
```

---

## UI Mockups (HTML/CSS)

Static UI mockups live in [`/example`](example/) and are published with GitHub Pages using `.github/workflows/pages.yml`.

- Entry page: `example/index.html`
- Included screens: Launcher, Timers, Recipes, Weather

### Local preview

Open `example/index.html` directly in a browser, or serve it locally:

```bash
cd example
python3 -m http.server 8000
```

Then open `http://localhost:8000`.

---

## Project History

| Version | Platform | Display |
|---------|----------|---------|
| KitchenGadget6000 | ESP32-S3, Arduino | 1.28" circular 240×240 (GC9A01) |
| KitchenGadget8000 | ESP32-P4, ESP-IDF | 9" IPS 1024×600 (RGB parallel) |

---

## License

MIT — see [LICENSE](LICENSE).
