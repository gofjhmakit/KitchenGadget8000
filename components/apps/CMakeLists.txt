idf_component_register(
    SRCS "LauncherApp.cpp" "ScreensaverApp.cpp" "TimersApp.cpp"
          "ConverterApp.cpp" "IngredientScalerApp.cpp" "BreadHydrationApp.cpp"
          "DrinkChillerApp.cpp" "MeatTemperaturesApp.cpp" "DishwasherApp.cpp"
          "WeatherApp.cpp" "RecipesApp.cpp" "LightingApp.cpp"
          "ShoppingListApp.cpp" "NotesApp.cpp"
    INCLUDE_DIRS "include"
    REQUIRES lvgl core ui services
)
