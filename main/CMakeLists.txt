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
    "app/storage.cpp"

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
    "ui/screens/study_screen.cpp"
    "ui/screens/focus_screen.cpp"
    "ui/screens/intermission_screen.cpp"
    "ui/screens/circle_screen.cpp"
    "ui/screens/pet_screen.cpp"
    "ui/screens/status_screen.cpp"
    "ui/screens/wardrobe_screen.cpp"
    "ui/screens/playground_screen.cpp"
    "ui/screens/touch_test_screen.cpp"

    INCLUDE_DIRS 
    "."
)

# Which demo board the firmware is wired for. 1 = test board one (rotary encoder),
# 2 = compact board (no encoder, buzzer on GPIO 2, touch IRQ on 15, backlight
# fixed at 3V3):
#   idf.py build            -> 1 (default)
#   idf.py -DBOARD=2 build  -> 2
if(NOT BOARD)
    set(BOARD 1)
endif()
if(BOARD != 1 AND BOARD != 2)
    message(FATAL_ERROR "BOARD must be 1 (test board one) or 2 (compact board), got ${BOARD}")
endif()
set(CMAKE_C_COMPILE_DEFINITIONS "${CMAKE_C_COMPILE_DEFINITIONS};BOARD=${BOARD}")
