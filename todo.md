# KitchenGadget8000 — Planned Features

This document contains detailed implementation specifications for the next batch of apps.
All apps follow the existing conventions: inherit `core::IApp`, use LVGL v9 API, apply the
gold/black theme, persist data through `core::Storage` (SPIFFS), and integrate with the
existing `services::Recipe` / `services::MarkdownParser` pipeline where applicable.

Each section includes: **Purpose**, **AppId**, **Data model**, **SPIFFS persistence**,
**UI layout**, **Recipe integration**, **Inter-app integration**, and **Step-by-step
implementation notes**.

---
## 0. Short ideas:

- Add weather to screensaver (use Muurame, Finland as default location).
- Add beautiful and stylish electricity price chart to screensaver too (Finnish electricity price, copy [from Disobey badge project](https://github.com/gofjhmakit/disobey_electricity_badge)).  

## 1. Meal Planner (Weekly)

### Purpose
Eliminate daily decision fatigue. The user plans the week's meals on Sunday; the device
guides every cooking session and, critically, feeds planned recipe ingredients directly
into the Shopping List so nothing is bought twice or forgotten.

### AppId
Add `MEAL_PLANNER` to the `core::AppId` enum in `core/AppManager.h` before `COUNT`.

### Data model

```cpp
// components/apps/include/apps/MealPlannerApp.h
struct MealSlot {
    char recipe_path[128]; // SPIFFS path to the .md file, empty = unset
    char custom_label[64]; // fallback text if no recipe is linked
    bool done;             // meal has been cooked/eaten this week
};

struct WeekPlan {
    // 7 days × 3 meals (Breakfast=0, Lunch=1, Dinner=2)
    MealSlot slots[7][3];
    uint8_t week_offset; // 0 = current ISO week; wrap-safe
};
```

### SPIFFS persistence
Save to `/meal_plan.bin` as a raw binary blob of `WeekPlan`.  Use `core::Storage::write_file`
/ `read_file`. On first launch write a zero-initialised struct.  Bump `week_offset` on Monday
00:00 (detected in `on_update` by comparing `TimeService::instance().week_number()` to stored
value); when a new week starts, copy the plan forward if "repeat weekly" flag is set (add
`bool repeat_weekly` to `WeekPlan`), otherwise clear all slots.

### UI layout (1024 × 600)

**List screen** (default view):
- Section title "Weekly Meal Plan" (gold, `font_title`).
- 7-column × 3-row grid.  Columns = Mon–Sun (abbreviated), rows = Breakfast / Lunch / Dinner.
- Each cell is a `ui::create_card` tap target.
  - If `done`: card background `#1E1A10`, label struck-through gold-dim text.
  - If recipe linked: show `recipe.image` emoji (large) + `recipe.title` (body font, wrapping).
  - If custom label: show ✏ emoji + label text.
  - If empty: show `+` in gold-dim, placeholder text "Tap to plan".
- Header row shows day abbreviations in `TEXT_SEC`.
- Tapping a cell opens the **slot editor** (modal card, 480 × 520 px, centered, gold border).

**Slot editor modal**:
- Title: e.g. "Wednesday · Lunch".
- Button row: [Browse Recipes] [Custom label] [Clear slot].
- "Browse Recipes" opens a sub-list identical to `RecipesApp::show_list()` (reuse logic,
  pass a callback). On selection: store `recipe.source_path` in `slot.recipe_path`, close modal.
- "Custom label" shows a single-line `lv_textarea` + numpad-less keyboard stub (or
  on-screen keyboard via `lv_keyboard_create`). On confirm: store text in `custom_label`.
- "Clear slot": zero the slot, close modal.
- Checkbox "Mark as done" at the bottom.

**Action bar** (below the grid):
- [Add all to Shopping List] — iterates every linked recipe in the plan, loads each
  `services::Recipe` via `MarkdownParser`, and appends missing ingredients to
  `ShoppingListApp::items_` (check for duplicates by lowercase comparison of `item.text`),
  then calls `ShoppingListApp::save()` and shows a success `Notifications::push`.
- [Open in Recipes] — when user taps a filled cell a second time, navigates to
  `RecipesApp` and calls `show_detail(index)` for the matching recipe index.

### Recipe integration
- When `[Add all to Shopping List]` is pressed: load each linked recipe from SPIFFS with
  `MarkdownParser::instance().parse_file(slot.recipe_path, recipe)`, iterate
  `recipe.ingredients`, append each as a `ShoppingListItem{text=ingredient, category="Pantry",
  done=false}`.  Ingredients already present in the list (case-insensitive match) are skipped.
- When a meal cell is tapped while `done == false` and a recipe is linked: offer
  [Cook Now] shortcut that navigates to `RecipesApp` and pre-selects the linked recipe (set
  `RecipesApp::current_index_` via a new `RecipesApp::open_recipe_by_path(const std::string&)`
  helper method and call `Navigation::navigate_to(AppId::RECIPES, Transition::SLIDE_LEFT)`).

### Inter-app integration
- **Timers**: no direct link; handled naturally when user cooks from RecipesApp.
- **Shopping List**: `[Add all to Shopping List]` action described above.
- **Recipes**: `[Cook Now]` action described above.

### Implementation steps
1. Add `MEAL_PLANNER` to `AppId` enum and define `MealSlot` / `WeekPlan` structs in header.
2. Implement `MealPlannerApp::on_mount` → load binary plan from SPIFFS, build grid UI.
3. Implement slot-tap handler → open modal, wire [Browse Recipes] sub-list, [Custom label]
   text entry, [Clear], [Done] checkbox.
4. Implement `on_update` → weekly rollover logic using `TimeService::week_number()`.
5. Implement `[Add all to Shopping List]` action with duplicate-skip logic.
6. Add `RecipesApp::open_recipe_by_path(const std::string&)` helper.
7. Register app in `main.cpp`, add launcher entry `{AppId::MEAL_PLANNER, "Meal Plan", "📅"}`.

---

## 7. Smart Substitutions Assistant

### Purpose
Keep cooking moving when an ingredient is unavailable. A user mid-recipe should be able to
tap any ingredient, see valid substitutes with quantity ratios and flavor-impact notes, and
optionally add the substitute to the Shopping List for next time.

### AppId
Add `SUBSTITUTIONS` to `core::AppId`.

### Data model

```cpp
// Compiled into a static constexpr table in SubstitutionsApp.cpp
struct Substitute {
    const char* original;   // normalised lowercase, e.g. "butter"
    const char* replacement; // e.g. "coconut oil"
    const char* ratio;       // e.g. "1:1 by weight"
    const char* note;        // e.g. "Adds faint coconut flavour; works best in baking"
};
```

Compile a table of at least 60 common substitutions covering:
dairy (butter, milk, cream, eggs), flours (plain, self-raising, almond), sweeteners
(sugar, honey, maple syrup), fats (oil, lard), acids (buttermilk, lemon juice, vinegar),
and protein binders (eggs → flax egg, chia egg). Store as `constexpr std::array` — no
SPIFFS needed (read-only data lives in flash).

### UI layout

**App entry — search screen**:
- Section title "Ingredient Substitutions".
- Large search `lv_textarea` (one-line, placeholder "Type ingredient…") at the top, full width.
- As the user types, the result list below filters live (in `on_update`, compare typed text
  to `substitute.original` with case-insensitive `strncasecmp`).
- Result list: scrollable column of `ui::create_list_item` rows.
  - Primary text: "Replace **{original}** with **{replacement}**".
  - Subtitle: ratio string.
  - Tapping a row opens the detail card (modal or inline expanded card).

**Detail card** (modal, 500 × 380 px):
- Title: original ingredient name (gold, `font_title`).
- Sub-heading "Use instead:" followed by replacement name.
- Ratio row: "Ratio: 1 : 1 by weight" with a small scale icon.
- Notes paragraph: full `substitute.note` text, word-wrapped.
- Button row: [Add replacement to Shopping List] [Close].

**"Add replacement to Shopping List"**:
- Appends `{substitute.replacement}` as a `ShoppingListItem{category="Pantry", done=false}`
  and shows a success toast.

### Recipe integration
- `RecipesApp::show_detail()` should be extended: add a small "⇄ Sub?" button next to each
  ingredient checkbox.  Tapping it navigates to `SubstitutionsApp` and pre-populates the
  search field with the ingredient name.  Implement via a new
  `SubstitutionsApp::search_for(const std::string& ingredient)` setter method that is called
  before navigation:
  ```cpp
  SubstitutionsApp::instance().search_for(ingredient_text);
  Navigation::instance().navigate_to(AppId::SUBSTITUTIONS, Transition::SLIDE_LEFT);
  ```
- When navigating back from `SubstitutionsApp` with a chosen substitute, the ingredient
  field in RecipesApp remains checked/unchanged (substitution is informational only).

### Inter-app integration
- **Shopping List**: [Add to Shopping List] button in detail card.
- **Recipes**: per-ingredient "Sub?" shortcut.

### Implementation steps
1. Create `SubstitutionsApp.h/.cpp`; add `AppId::SUBSTITUTIONS`.
2. Define `constexpr std::array<Substitute, N> kSubstitutions` in `.cpp`.
3. Build search-screen UI; wire textarea change event to filter logic.
4. Build detail modal UI; wire [Add to Shopping List].
5. Add `search_for(const std::string&)` setter; update `RecipesApp` ingredient row rendering.
6. Register app; add launcher entry `{AppId::SUBSTITUTIONS, "Subs", "⇄"}`.

---

## 9. Multi-Dish Cooking Timeline

### Purpose
When cooking a full meal with 3–5 dishes, the user often misjudges start times and
everything finishes at different moments. This app creates a synchronised schedule so all
dishes land on the table at the same time.

### AppId
Add `COOKING_TIMELINE` to `core::AppId`.

### Data model

```cpp
struct TimelineDish {
    char label[64];         // user-entered dish name or recipe title
    char recipe_path[128];  // optional linked recipe .md path
    uint32_t cook_minutes;  // total cooking time
    uint32_t start_offset;  // minutes before serve time to start (computed)
    bool timer_started;     // true once the "Start" button has been tapped
    int timer_slot;         // -1 if none, else 0-4 (TimersApp slot index)
};

struct TimelinePlan {
    TimelineDish dishes[8]; // up to 8 simultaneous dishes
    uint8_t dish_count;
    uint32_t serve_time_epoch; // Unix timestamp of target serve time
};
```

Persist to `/timeline.bin` so an in-progress plan survives app switches.

### UI layout

**Setup screen** (first launch or when no active timeline):
- Section title "Cooking Timeline".
- "Serve time" row: H:MM picker built from two `lv_roller` widgets (hours 0–23, minutes
  0–55 in 5-min steps); defaults to now + 1 h.
- "Add dish" section: textarea for label, number input for minutes, optional [Pick from
  Recipes] button.  [+ Add] appends to the dish list.
- Dish list: each row shows `dish.label`, `dish.cook_minutes` min, delete button.
- [Generate Schedule] button at the bottom.

**Schedule screen** (after [Generate Schedule]):
- Backward-scheduling algorithm: `start_offset = serve_time - cook_minutes`. Sort dishes by
  start_offset ascending.
- Display as vertical timeline (scrollable column):
  - Each dish is a `ui::create_card` with:
    - Start time (HH:MM, gold, `font_title`).
    - Dish name (body font, white).
    - Cook duration (gold-dim).
    - Status pill: `WAITING` (dim) / `START NOW` (gold pulsing) / `COOKING` (green arc) /
      `DONE` (struck-through dim).
  - A vertical gold line runs between cards as a timeline rail.
- [Start All] button: adds a `TimersApp` timer for every dish starting at `start_offset`
  from now (calls `TimersApp::add_timer(cook_minutes * 60, dish.label, "🍽")`).
- Each dish card also has [Start] / [Done] tap actions.
- `on_update` ticks a countdown to the next start event and highlights the card when it
  is time to start cooking ("START NOW" state + gold pulse animation).

### Recipe integration
- [Pick from Recipes]: opens a mini recipe browser (same pattern as Meal Planner sub-list).
  On selection: populate `dish.label = recipe.title`, `dish.cook_minutes` parsed from
  `recipe.time` field (e.g. "25 min" → 25).  Parser helper:
  `static uint32_t parse_time_minutes(const std::string& time_str)` — extract leading integer.
- When a recipe is linked (`recipe_path` set) a [View Recipe] link appears on the dish card;
  tapping navigates to `RecipesApp::open_recipe_by_path`.

### Inter-app integration
- **Timers**: [Start All] / [Start] creates TimersApp timers automatically.
- **Recipes**: [Pick from Recipes] link and [View Recipe] link.

### Implementation steps
1. Create `CookingTimelineApp.h/.cpp`; add `AppId::COOKING_TIMELINE`.
2. Implement setup screen: serve-time rollers, dish entry form, recipe picker link.
3. Implement backward-scheduler: sort by start offset, compute `start_offset` per dish.
4. Render timeline cards with status logic.
5. Implement `on_update` to track elapsed time, fire "START NOW" state transitions.
6. Wire [Start All] / [Start] to `TimersApp::add_timer`.
7. Wire recipe link to `RecipesApp::open_recipe_by_path`.
8. Persist to SPIFFS; load on mount.
9. Register app; add launcher entry `{AppId::COOKING_TIMELINE, "Timeline", "📋"}`.

---

## 10. Oven & Hob Assistant

### Purpose
Guide the user through the mechanical but error-prone actions of oven and hob management:
preheating in time, tracking burner states, and alerting to carryover cooking.  This prevents
the most common causes of burned or undercooked food.

### AppId
Add `OVEN_HOB` to `core::AppId`.

### Data model

```cpp
enum class OvenMode { OFF, PREHEAT, HOLDING };
enum class HobState { OFF, LOW, MEDIUM, HIGH };

struct OvenState {
    OvenMode mode;
    uint16_t target_temp_c;  // user-set target temperature
    uint8_t  preheat_minutes; // estimated preheat time (rule: 10 min + 1 min per 10°C above 100)
    uint32_t preheat_start_epoch; // Unix timestamp when preheat timer was started
    bool     preheat_notified;
};

struct HobBurner {
    HobState state;
    char     label[32]; // e.g. "Front left" or linked dish name
    uint32_t on_since_epoch;
};

struct OvenHobData {
    OvenState oven;
    HobBurner burners[4];
};
```

Persist to `/oven_hob.bin`; reset oven `mode` to `OFF` on cold boot (compare boot epoch
to stored `preheat_start_epoch`; if delta > 2 h, treat as new session).

### UI layout

**Main screen** — two panels side by side (50/50 split):

**Left panel — Oven**:
- Card with oven icon 🔥 and title "Oven".
- Temperature row: three quick-select buttons (160 °C / 180 °C / 200 °C) + custom numpad
  entry. Display in both °C and °F (reuse `ConverterApp` conversion math).
- [Preheat Now] button: starts a `TimersApp` timer labelled "Preheat" for the computed
  `preheat_minutes`. Sets `oven.mode = PREHEAT` and stores epoch.
- Status row: "Preheat: 12 min remaining" (counts down in `on_update`) or "Ready ✓" (gold)
  when timer expires.
- Carryover alert toggle (checkbox): when checked, after the oven timer fires the app
  pushes a notification "Remove food — carryover cooking continues for ~5 min".
- [Oven Off] button: resets mode.

**Right panel — Hob**:
- 2×2 grid of burner cards (Front-Left, Front-Right, Back-Left, Back-Right).
- Each card: burner label (editable via long-press), state selector row
  (OFF | ○ Low | ◑ Med | ● High shown as segmented toggle), elapsed time since turned on.
- Elapsed time ticks in `on_update`; colour-codes: <5 min (white), 5–15 min (gold),
  >15 min (orange), >30 min (red pulse) — visual reminder of overlong heat.
- Long-pressing a burner card opens a small modal to rename the label (e.g. tie to a dish).

**Action bar**:
- [Quick Start from Recipe] — opens recipe browser; on selection reads first instruction
  containing a temperature keyword (e.g. "200°C", "180°C") and pre-fills the oven temp
  field, and first instruction containing "preheat" sets a preheat timer automatically.

### Recipe integration
- [Quick Start from Recipe] described above.
- Temperature extraction helper:
  `static uint16_t extract_temp_celsius(const std::string& instruction)` —
  scan for patterns `NNN°C`, `NNN°F` (convert), `NNN degrees` etc. using the same
  digit-scan loop pattern already used in `MarkdownParser::extract_timers_from_step`.
- When a recipe is active in `RecipesApp` (track via `RecipesApp::current_index_` + a
  new `RecipesApp::current_recipe()` accessor), display a banner on OvenHob's main screen:
  "Active recipe: {title}" with [Open Recipe] shortcut.

### Inter-app integration
- **Timers**: [Preheat Now] creates a timer in `TimersApp`.
- **Converter**: oven temp shown in °C + °F simultaneously using converter math.
- **Recipes**: [Quick Start from Recipe] link.

### Implementation steps
1. Create `OvenHobApp.h/.cpp`; add `AppId::OVEN_HOB`.
2. Build two-panel LVGL layout with oven card and 2×2 burner grid.
3. Implement preheat timer estimation formula; wire [Preheat Now] to `TimersApp::add_timer`.
4. Implement `on_update`: countdown display, burner elapsed-time colour logic.
5. Implement carryover alert notification.
6. Add `extract_temp_celsius` helper; wire [Quick Start from Recipe] browser.
7. Add `RecipesApp::current_recipe()` accessor returning `const services::Recipe*`.
8. Register app; add launcher entry `{AppId::OVEN_HOB, "Oven/Hob", "🔥"}`.

---

## 11. Batch Cooking & Leftover Planner

### Purpose
Help users who cook once and eat multiple times across the week. The app tracks portions,
assigns meals from cooked batches, and reminds users when leftovers are approaching their
safe-storage deadline.

### AppId
Add `BATCH_PLANNER` to `core::AppId`.

### Data model

```cpp
struct BatchEntry {
    char label[64];          // dish name or recipe title
    char recipe_path[128];   // optional linked recipe .md path
    uint8_t total_portions;  // how many portions were made
    uint8_t portions_left;   // decremented each time user logs a portion eaten
    uint32_t cooked_epoch;   // Unix timestamp when batch was made
    uint8_t fridge_days;     // safe storage in fridge (default 4, editable)
    uint8_t freezer_days;    // safe storage if frozen (default 90, editable)
    bool frozen;             // true = stored in freezer
    bool notified_expiry;    // flag so expiry alert fires only once
};

// Persisted as a JSON-like delimited text file for readability
// Format per line: cooked_epoch|label|recipe_path|total|left|fridge_days|freezer_days|frozen|notified
```

Persist to `/batches.txt`.  Load/save use `core::Storage::read_file` / `write_file` with
the `|`-delimited format (same pattern as `ShoppingListApp`).

### UI layout

**Dashboard screen**:
- Section title "Batch Cooking".
- Top strip: "Eat first" priority list — batches sorted by `(cooked_epoch + safe_days * 86400)`
  ascending; the nearest-expiring batch shown as a gold highlight card at the top.
- Main list: each batch shown as a `ui::create_card` row:
  - Left: emoji (from linked recipe if available) + label.
  - Middle: portion pill "3 / 6 portions left" (gold number / dim denominator).
  - Right column: "Fridge: 2 days left" or "Freezer: 88 days left" (colour-coded).
  - [+] / [−] buttons to adjust `portions_left`.
  - Long-press → edit modal (change label, portions, storage type, days).
- [+ Log new batch] button opens an entry form.

**Entry form** (modal 480 × 500 px):
- Label textarea (or [Pick from Recipes]).
- "Portions" numpad entry.
- "Storage" toggle: [Fridge] / [Freezer].
- "Safe days" input (pre-filled from defaults; user can adjust).
- [Save] writes a new `BatchEntry` with `cooked_epoch = time(nullptr)`.

**"Eat from this batch" flow**:
- Tapping [−] decrements `portions_left` and saves.  If `portions_left == 0`, auto-move
  entry to a "Finished" section (visually dimmed) and push a success toast.

**Expiry alerts**:
- `on_update` (check once per minute via accumulator): if `days_remaining <= 1` and
  `!notified_expiry`, push `Notifications::push(ALARM, "Use today!", label)` and set flag.

### Recipe integration
- [Pick from Recipes] in entry form: pre-fills `label` from `recipe.title`, sets
  `recipe_path`, and imports `recipe.servings` as the default portion count.
- Tapping a batch card with `recipe_path` set shows a [View Recipe] button that calls
  `RecipesApp::open_recipe_by_path` — useful for reheating instructions.
- When `portions_left <= 1`, optionally suggest a "use-it-up" recipe from the existing
  recipe library by checking `recipe.tags` for matching ingredient keywords in the batch
  label (basic substring match — no heavy search needed).

### Inter-app integration
- **Recipes**: [Pick from Recipes] entry, [View Recipe] card link.
- **Shopping List**: when `portions_left == 0` (batch finished), offer [Re-stock: add to
  Shopping List] which appends the batch label as a `ShoppingListItem`.
- **Meal Planner**: (optional) when a batch exists for a dish, show a "🥡 Leftovers
  available" indicator on matching Meal Planner slots (future enhancement, not required
  for v1).

### Implementation steps
1. Create `BatchPlannerApp.h/.cpp`; add `AppId::BATCH_PLANNER`.
2. Implement pipe-delimited load/save (mirror `ShoppingListApp` style).
3. Build dashboard UI: priority "eat first" card, scrollable batch list.
4. Implement [+ Log new batch] modal with recipe picker integration.
5. Wire [+] / [−] portion buttons; handle zero-portion finished state.
6. Implement expiry alert in `on_update` (60 s accumulator pattern).
7. Add [View Recipe] card link; add [Re-stock] shopping list link.
8. Register app; add launcher entry `{AppId::BATCH_PLANNER, "Batches", "🥡"}`.

---

## 13. Seasonal Produce Guide

### Purpose
Help users shop smarter and cook tastier food by knowing what is in season right now.
Seasonal produce is cheaper, more nutritious, and better-tasting. The guide also pivots
directly into relevant recipes from the existing library.

### AppId
Add `SEASONAL_GUIDE` to `core::AppId`.

### Data model

```cpp
struct ProduceItem {
    const char* name;
    const char* emoji;
    const char* category;   // "Vegetable", "Fruit", "Herb"
    const char* tip;        // short note, e.g. "Choose firm courgettes under 20 cm"
    uint8_t months;         // bitmask: bit 0 = Jan, bit 11 = Dec
};
```

Implement as a `constexpr std::array<ProduceItem, N>` in `SeasonalGuideApp.cpp`.  Cover
at least 50 items across Northern European / Northern American temperate seasons
(the target audience implied by the default weather coordinates in `sdkconfig.defaults`).
`months` bitmask allows multi-month items without runtime allocation.

Month is read from `services::TimeService::instance()` — add a `uint8_t current_month()`
method (1-based, returning `tm.tm_mon + 1`).

### UI layout

**Home screen — This Month**:
- Section title "In Season — {Month Name}" (e.g. "In Season — July").
- Horizontal scrollable chip row showing category filters: [All] [Vegetables] [Fruits]
  [Herbs].  Active chip has gold fill.
- Produce list below: each item as a `ui::create_list_item` row with emoji + name +
  category subtitle.  Tapping opens the detail card.

**Detail card** (full-screen replace, not modal — uses Navigation):
- Large emoji (48 pt).
- Name (gold, `font_title`).
- Category badge.
- "Season tip" paragraph (body font, `TEXT_SEC`).
- Divider.
- "Recipes featuring this ingredient" section:
  - On mount, iterate all loaded `RecipesApp::recipes_` and check if any
    `recipe.ingredients` entry contains the produce name (case-insensitive substring match).
  - List matching recipes as compact cards with [Cook this] tap target that navigates to
    `RecipesApp` and opens the recipe.
- If no matches: show "No recipes found — try searching manually" with a [Go to Recipes]
  button.
- [Back] returns to the month home screen.

**Calendar overview** (accessible via a "View full calendar" button):
- 12-column × N-row grid. Columns = Jan–Dec (abbreviated). Rows = produce items.
- Cell background gold if the item is in season that month; black otherwise.
- This is a read-only reference table; scroll vertically to browse all items.

### Recipe integration
- Detail card "Recipes featuring this ingredient" section described above.
  Implementation: on navigation to detail, call `RecipesApp::recipes()` (add a
  `const std::vector<services::Recipe>& recipes() const` accessor) and filter by substring
  match of `produce.name` against each `recipe.ingredients` vector.
- [Cook this] in detail card: `RecipesApp::open_recipe_by_path(recipe.source_path)` then
  navigate.

### Inter-app integration
- **Recipes**: detail card ingredient-matching and [Cook this] shortcut.
- **Shopping List**: [+ Add to shopping list] button on each detail card; appends
  `{produce.name}` in category "Produce".

### Implementation steps
1. Create `SeasonalGuideApp.h/.cpp`; add `AppId::SEASONAL_GUIDE`.
2. Compile `kProduceItems` constexpr array (50+ items with month bitmasks).
3. Add `TimeService::current_month()` accessor.
4. Build home screen: title with live month name, category filter chips, filtered list.
5. Build detail card screen with tip text, recipe matching section, [Cook this], [+ Add].
6. Add `RecipesApp::recipes()` const accessor.
7. Build calendar overview grid (scrollable, read-only).
8. Register app; add launcher entry `{AppId::SEASONAL_GUIDE, "Seasonal", "🌱"}`.

---

## 14. Fermentation & Long-Cook Tracker

### Purpose
Fermentation, bread starters, slow braises, marinating, and curing span hours to weeks.
Standard kitchen timers are not designed for this. This app provides stage-aware,
multi-day progress tracking with checkpoint reminders and safety guidance, deeply
integrated with recipes that include long-process instructions.

### AppId
Add `FERMENTATION_TRACKER` to `core::AppId`.

### Data model

```cpp
enum class FermentStage { PREP, ACTIVE, CHECK, REST, DONE };

struct FermentCheckpoint {
    char label[64];           // e.g. "First fold", "Bulk ferment start", "Score & bake"
    uint32_t offset_hours;    // hours after `start_epoch` this checkpoint is due
    bool completed;
    char safety_note[128];    // e.g. "Discard if any pink/orange mould is visible"
};

struct FermentProject {
    char name[64];            // e.g. "Sourdough #3", "Kimchi batch"
    char recipe_path[128];    // optional linked recipe .md path
    FermentStage stage;
    uint32_t start_epoch;     // Unix timestamp when project was started
    uint32_t total_hours;     // expected total duration
    FermentCheckpoint checkpoints[8];
    uint8_t checkpoint_count;
    uint8_t temp_celsius;     // ambient temperature at start (affects ferment speed)
    bool notified_overdue;    // flag to prevent repeated alerts
};
```

Persist to `/ferment_projects.txt` as a multi-line record format (one project per block,
separated by `---`).  Alternatively use a simple binary blob: `FermentProject[8]` with
a leading count byte.  Binary is simpler; use `/ferment.bin`.

### UI layout

**Dashboard screen**:
- Section title "Fermentation & Long Cooks".
- Active projects list: each shown as a `ui::create_card`:
  - Project name (gold, `font_title`).
  - Progress arc ring (same style as `TimersApp`): percentage of total hours elapsed.
  - Stage badge: coloured pill (PREP=dim, ACTIVE=gold, CHECK=orange, REST=blue-grey,
    DONE=green).
  - Next checkpoint row: label + hours remaining (e.g. "First fold — in 2 h 15 min").
  - Tapping the card opens the **project detail** screen.
- [+ New Project] button at the bottom.

**Project detail screen** (full-screen, uses Navigation):
- Header: project name + linked recipe shortcut.
- Temperature influence note: if `temp_celsius` is set, show a small advisory:
  "At {N}°C expect faster/slower fermentation" (rule: base at 21°C; +1°C ≈ 10% faster).
- Checkpoint timeline: vertical list of `FermentCheckpoint` cards:
  - Due time shown as both absolute (HH:MM on day N) and relative ("in 3 h").
  - [Mark Done] button per checkpoint; tapping sets `completed = true`, saves, and pushes
    a success notification.
  - Safety note shown below each step in `TEXT_SEC` colour.
  - Next due checkpoint highlighted with gold border + pulse animation.
- [Abandon project] button (long-press confirm): removes from active list.

**New project form** (modal):
- Name textarea.
- [Pick from Recipes] — see Recipe integration below.
- Duration: days/hours rollers.
- Ambient temperature slider (10–35°C).
- [Generate checkpoints] button: for generic projects, auto-populates sensible
  fermentation checkpoints based on duration (e.g. for 24 h: fold at 2 h, bulk at 4 h,
  shape at 12 h, proof at 22 h, bake at 24 h).
- Manual checkpoint editor: [+ Add checkpoint] row per checkpoint with label textarea +
  hour-offset numpad.
- [Start] saves the project with `start_epoch = time(nullptr)`.

**Overdue alerts** (in `on_update`, 60 s accumulator):
- For each uncompleted checkpoint whose due time has passed: push
  `Notifications::push(ALARM, "Action needed", checkpoint.label + " — " + project.name)`
  and set `notified_overdue = true` per checkpoint.

### Recipe integration
- [Pick from Recipes] in new project form: load recipe, populate `name = recipe.title`,
  `recipe_path`.
- After selecting a recipe, scan `recipe.instructions` for time-denoting patterns:
  `MarkdownParser::extract_timer_seconds` extended (or a new local helper) to also match
  day-scale patterns ("overnight", "24 hours", "3 days").  Map found durations to
  checkpoint `offset_hours` values automatically.  Pre-populate checkpoint labels from
  the instruction text (truncated to 60 chars).
- Recipe instruction parsing for days: add
  `static uint32_t extract_duration_hours(const std::string& instruction)` helper in
  `MarkdownParser` (or locally in `FermentationTrackerApp.cpp` to avoid modifying the
  shared service for a specialised use-case).
- On the project detail screen, [View Recipe] shortcut navigates to `RecipesApp`.
- When the project stage advances to `DONE`, push a "Finished!" notification and offer
  [Log as batch] which creates a new `BatchEntry` in `BatchPlannerApp` (if that app is
  registered) — a natural handoff from fermentation to leftover tracking.

### Inter-app integration
- **Timers**: from the project detail screen, a [Set reminder timer] action lets the user
  create a short `TimersApp` timer for the next checkpoint (useful for the next hour or
  two; not a substitute for the multi-day tracking).
- **Recipes**: [Pick from Recipes], auto-checkpoint generation from instructions, [View
  Recipe] shortcut.
- **Batch Planner**: [Log as batch] on project completion.
- **Notifications**: checkpoint overdue alerts and daily digest ("You have 2 ferments
  active — next action in 1 h").

### Implementation steps
1. Create `FermentationTrackerApp.h/.cpp`; add `AppId::FERMENTATION_TRACKER`.
2. Implement binary load/save for `FermentProject[8]`.
3. Build dashboard with progress arcs (reuse `ui::create_progress_ring`).
4. Build project detail screen: checkpoint timeline, [Mark Done], safety notes.
5. Build new project form: recipe picker, duration rollers, temperature slider,
   auto-checkpoint generator, manual editor.
6. Implement `on_update` overdue alert logic.
7. Add `extract_duration_hours` helper; wire recipe auto-checkpoint population.
8. Wire [Log as batch] to `BatchPlannerApp` (if registered, dynamic cast check).
9. Wire [Set reminder timer] to `TimersApp::add_timer`.
10. Register app; add launcher entry `{AppId::FERMENTATION_TRACKER, "Ferments", "🫙"}`.

---

## 20. Emergency Cooking School (Skill Coach)

### Purpose
An embedded micro-learning system for users who lack confidence in the kitchen.  Rather than
a full course, it delivers sharp, actionable, point-of-need guidance on fundamental
techniques — knife work, checking doneness, rescuing common mistakes, sauce-making.  The
target user is standing at the counter, hands possibly wet, and needs the answer in
< 30 seconds of reading.

### AppId
Add `SKILL_COACH` to `core::AppId`.

### Data model

```cpp
struct CoachStep {
    const char* text;        // instruction text, max ~180 chars, plain
    uint32_t timer_seconds;  // 0 = no timer; else shows [Start timer] button
};

struct CoachLesson {
    const char* title;
    const char* emoji;
    const char* category;    // "Knife Skills", "Doneness", "Rescue", "Sauces", "Baking"
    const char* summary;     // one-sentence hook shown in the list
    const CoachStep* steps;
    uint8_t step_count;
};
```

Store all lessons as `constexpr` static data in `SkillCoachApp.cpp`.  No SPIFFS needed.
Aim for at least 20 lessons covering:
- **Knife Skills**: proper grip, the claw technique, uniform dice, julienne basics,
  herbs (chiffonade), mincing garlic.
- **Doneness checks**: steak finger-press test (rare/medium/well), fish flake test,
  chicken juice test, bread hollow-knock, pasta bite-test (al dente vs. overcooked).
- **Rescue**: broken emulsion (mayo/hollandaise), over-salted dish, burnt base, broken
  béchamel, deflated sponge, curdled cream sauce.
- **Sauces**: basic pan sauce (deglaze, reduce, mount), emulsified butter sauce, simple
  vinaigrette ratios.
- **Baking basics**: cold vs. room-temp butter, folding vs. stirring, testing cake doneness,
  proofing dough (poke test).

Track per-lesson completion in NVS (`core::Settings` or a small separate NVS namespace
`"skill_coach"`) with a `uint32_t completed_bitmask` (supports up to 32 lessons; extend
to `uint64_t` for 64 if needed).

### UI layout

**Home screen**:
- Section title "Skill Coach".
- Category filter chips: [All] [Knife] [Doneness] [Rescue] [Sauces] [Baking].
- Lesson list: each as a `ui::create_list_item` with emoji + title + summary subtitle.
  Completed lessons show a gold ✓ badge on the right.
- Search is NOT required — the list is short enough to scroll; add a simple text filter
  textarea at the top as a nice-to-have.

**Lesson screen** (full-screen):
- Header: emoji (48 pt, centered) + lesson title (gold, `font_title`).
- Category badge pill (gold-dim background, gold text).
- Horizontal page indicator (e.g. "Step 2 of 5") using small gold dots.
- Step content card: white body text on dark card, word-wrapped, generous padding.
- If `step.timer_seconds > 0`: show [Start {N}-min timer] gold button that calls
  `TimersApp::add_timer(step.timer_seconds, lesson.title, lesson.emoji)` and navigates
  to `TimersApp`.
- [Previous] / [Next] navigation row at the bottom.
- On final step: [Mark Complete] button — sets the lesson's bit in `completed_bitmask`,
  saves to NVS, pushes a success toast "Lesson complete ✓", and returns to home screen.
- [Return to list] always available top-left (gold back arrow label).

**Progress summary** (accessible via "My Progress" button on home screen):
- "Lessons completed: X / Y" arc ring (same style as `TimersApp`).
- Category breakdown: small bars showing e.g. "Knife: 3/6 | Doneness: 1/4 …".
- [Reset progress] button (long-press confirm).

### Recipe integration
- On the **Recipes** detail screen, add a contextual [📚 Tip] button in the Instructions
  section that appears when a step contains technique keywords (e.g. "dice", "julienne",
  "deglaze", "fold", "knead", "proof", "sear").
- Keyword detection: maintain a `constexpr` mapping `{keyword → AppId::SKILL_COACH + lesson_index}`
  in `RecipesApp.cpp`.  When a matching keyword is found in an instruction step, render
  a small gold [📚 Tip] button alongside the step text.
- Tapping [📚 Tip] sets `SkillCoachApp::pending_lesson_index_` and navigates to
  `AppId::SKILL_COACH`; the app opens directly to the named lesson rather than the home
  list.  Implement via `SkillCoachApp::open_lesson(uint8_t index)` setter.

### Inter-app integration
- **Timers**: [Start timer] in step content (e.g. "Rest dough for 10 minutes" → 10-min
  timer).
- **Recipes**: per-step [📚 Tip] keyword bridge described above.

### Implementation steps
1. Create `SkillCoachApp.h/.cpp`; add `AppId::SKILL_COACH`.
2. Author `constexpr` lesson and step arrays (20+ lessons, ~5 steps each).
3. Build home screen with category filter chips and lesson list.
4. Build lesson screen: step display, Previous/Next, Mark Complete, timer button.
5. Build progress summary screen with arc ring and category bars.
6. Persist `completed_bitmask` to NVS via `core::Settings` or direct `nvs_set_u32`.
7. Add `open_lesson(uint8_t index)` setter for external navigation.
8. Add technique-keyword detection and [📚 Tip] buttons to `RecipesApp::show_detail`.
9. Register app; add launcher entry `{AppId::SKILL_COACH, "Coach", "📚"}`.

---

## Cross-cutting notes for all new apps

### AppId registration
Each new app requires:
1. A new entry in the `core::AppId` enum **before `COUNT`** in
   `components/core/include/core/AppManager.h`.
2. `mgr.register_app(std::make_unique<apps::FooApp>())` in `main/main.cpp`.
3. An entry in `LauncherApp.cpp` `kApps` array. The launcher grid is 3-wide; with
   8 new apps added to the existing 12, the grid grows to 4 rows of 3 (20 apps total
   — one empty cell). The `rows[]` array in `LauncherApp::build_ui` should be extended
   to `{LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
       LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}` once all
   apps are registered.
4. `MyApp.cpp` added to `components/apps/CMakeLists.txt` `SRCS` list.

### Shared helpers to add
| Helper | Location | Used by |
|--------|----------|---------|
| `RecipesApp::open_recipe_by_path(const std::string&)` | `RecipesApp.cpp/.h` | Meal Planner, Cooking Timeline, Batch Planner, Seasonal Guide, Fermentation Tracker |
| `RecipesApp::current_recipe() const → const services::Recipe*` | `RecipesApp.h` | Oven/Hob |
| `RecipesApp::recipes() const → const std::vector<services::Recipe>&` | `RecipesApp.h` | Seasonal Guide |
| `TimeService::current_month() → uint8_t` | `TimeService.h/.cpp` | Seasonal Guide |
| `SkillCoachApp::open_lesson(uint8_t index)` | `SkillCoachApp.h` | Recipes App tip bridge |
| `SubstitutionsApp::search_for(const std::string&)` | `SubstitutionsApp.h` | Recipes App |
| `BatchPlannerApp::add_batch(BatchEntry)` | `BatchPlannerApp.h` | Fermentation Tracker |

### Style consistency
All new apps must:
- Use `ui::create_card`, `ui::create_gold_button`, `ui::create_section_title`,
  `ui::create_list_item` from `ui/Widgets.h`.
- Apply `ui::anim::fade_in` on mount for the staggered entry animation.
- Use `ui::Color::GOLD_HI` / `GOLD` / `GOLD_DIM` / `TEXT_PRI` / `TEXT_SEC` / `BG`.
- Use `ui::Theme::font_title()` for large numbers/headings, `font_body()` for body text.
- Call `core::PowerManager::instance().reset_activity()` on any significant user interaction.
- Use `core::Notifications::instance().push(type, title, body)` for all user feedback.
