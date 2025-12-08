#pragma once
#include <ui/screen.hpp>

class LockScreen : public Screen {
public:
    LockScreen(UIManager *ui) : Screen(ui) {}
    void render() override;
};