idf_component_register(
    LANGUAGE C CXX

    SRCS 
    "main.cpp"
    "board.c"
    "utilities.cpp"

    "hal/display.c" 
    "hal/touch.c" 
    "hal/encoder.c"
    "hal/buzzer.c"
    "system/system.cpp"
    "system/health.cpp"

    "app/session_core.cpp"
    "app/circle_core.cpp"
    "app/session.cpp"
    "app/storage.cpp"

    "comms/link.cpp"
    "comms/bt_spp.cpp"
    "comms/circle.cpp"

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
    "ui/screens/ready_screen.cpp"
    "ui/screens/circle_screen.cpp"
    "ui/screens/invite_screen.cpp"
    "ui/screens/pet_screen.cpp"
    "ui/screens/status_screen.cpp"
    "ui/screens/wardrobe_screen.cpp"
    "ui/screens/playground_screen.cpp"
    "ui/screens/touch_test_screen.cpp"

    INCLUDE_DIRS 
    "."
)

# idf.py -DPOMO_FAKE_PEER=1 build: scripted study circle friends, to test linked play on one
# board (comms/circle.hpp). Build with -DPOMO_FAKE_PEER=0 to turn it off again.
if(POMO_FAKE_PEER)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE POMO_FAKE_PEER=1)
endif()
