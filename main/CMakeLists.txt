idf_component_register(
    LANGUAGE C CXX

    SRCS 
    "main.cpp" 

    "hal/display.c" 
    "hal/touch.c" 
    "system/system.cpp"

    "ui/app_state.cpp"
    "ui/navigator.cpp"
    "ui/ui_root.cpp"

    "ui/screens/stubs.cpp"
    "ui/screens/home_screen.cpp"
    "ui/screens/lock_screen.cpp"

    INCLUDE_DIRS 
    "."
)