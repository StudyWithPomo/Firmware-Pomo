idf_component_register(
    LANGUAGE C CXX

    SRCS 
    "main.cpp" 
    "utilities.cpp"

    "hal/display.c" 
    "hal/touch.c" 
    "hal/encoder.c"
    "system/system.cpp"

    "app/session_core.cpp"
    "app/session.cpp"

    "comms/link.cpp"
    "comms/bt_spp.cpp"

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

    "ui/screens/boot_screen.cpp"
    "ui/screens/home_screen.cpp"
    "ui/screens/wind_screen.cpp"
    "ui/screens/focus_screen.cpp"
    "ui/screens/intermission_screen.cpp"
    "ui/screens/menu_screen.cpp"
    "ui/screens/workshop_screen.cpp"
    "ui/screens/circle_screen.cpp"
    "ui/screens/timer_settings_screen.cpp"
    "ui/screens/touch_test_screen.cpp"

    INCLUDE_DIRS 
    "."
)
