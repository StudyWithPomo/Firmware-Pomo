idf_component_register(
    LANGUAGE C CXX

    SRCS 
    "main.cpp" 
    "utilities.cpp"

    "hal/display.c" 
    "hal/touch.c" 
    "system/system.cpp"

    "ui/app_state.cpp"
    "ui/navigator.cpp"
    "ui/ui_root.cpp"

    "ui/assets/app_overlay.c"

    "ui/fonts/ext_symbols_14.c"
    "ui/fonts/ext_symbols_28.c"

    "ui/screens/stubs.cpp"
    "ui/screens/touch_test_screen.cpp"
    "ui/screens/home_screen.cpp"
    "ui/screens/lock_screen.cpp"
    "ui/screens/settings_screen.cpp"

    "ui/wallpapers/blue.c"
    "ui/wallpapers/blue_wave.c"
    "ui/wallpapers/gray_arc.c"
    "ui/wallpapers/light.c"

    INCLUDE_DIRS 
    "."
)