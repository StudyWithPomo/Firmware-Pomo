idf_component_register(
    LANGUAGE C CXX

    SRCS 
    "main.cpp" 
    "utilities.cpp"

    "hal/display.c" 
    "hal/touch.c" 
    "hal/encoder.c"
    "system/system.cpp"

    "ui/app_state.cpp"
    "ui/navigator.cpp"
    "ui/ui_root.cpp"
    "ui/theme.cpp"
    "ui/pet/pet_view.cpp"

    "ui/fonts/font_fell_12.c"
    "ui/fonts/font_fell_14.c"
    "ui/fonts/font_fell_16.c"
    "ui/fonts/font_fell_25.c"
    "ui/fonts/font_caslon_12.c"
    "ui/fonts/font_plex_12.c"
    "ui/fonts/font_plex_22.c"
    "ui/fonts/font_plex_28.c"
    "ui/fonts/font_plex_50.c"

    "ui/screens/home_screen.cpp"
    "ui/screens/placeholder_screen.cpp"
    "ui/screens/touch_test_screen.cpp"

    INCLUDE_DIRS 
    "."
)
