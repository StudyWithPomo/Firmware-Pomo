idf_component_register(
    LANGUAGE C CXX
    SRCS "main.cpp" "hal/display.c" "hal/touch.c" "ui/ui_manager.cpp" "ui/screens/home_screen.cpp" "ui/screens/lock_screen.cpp"
    INCLUDE_DIRS "."
)