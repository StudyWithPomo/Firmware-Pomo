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

    "ui/screens/placeholder_screen.cpp"
    "ui/screens/touch_test_screen.cpp"

    INCLUDE_DIRS 
    "."
)
