#pragma once

class UIManager;

class Screen {
public:
    Screen(UIManager *ui) : ui_manager(ui) {}
    virtual void render() = 0;     // pure virtual
    virtual ~Screen() = default;   // inline default destructor

protected:
    UIManager *ui_manager = nullptr;
};
