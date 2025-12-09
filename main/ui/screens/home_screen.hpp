#pragma once
#include <ui/screen.hpp>

class HomeScreen : public Screen {
public:
    static HomeScreen *getInstance() {
        static HomeScreen instance;
        return &instance;
    }
    void render() override;
};