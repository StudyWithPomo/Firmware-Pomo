#pragma once

class Screen;

class UIManager {
public:
    UIManager() = default;
    static UIManager& getInstance() {
        static UIManager instance;
        return instance;
    }

    void setScreen(Screen* s);
    void render();

private:
    Screen* current = nullptr;
};
