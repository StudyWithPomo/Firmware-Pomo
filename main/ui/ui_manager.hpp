#pragma once

class Screen;

class UIManager {
public:
    UIManager() = default;

    void setScreen(Screen* s);
    void render();

private:
    Screen* current = nullptr;
};
