#pragma once
#include <ui/screen.hpp>

class LockScreen : public Screen {
public:
    static LockScreen *getInstance() {
        static LockScreen instance;
        return &instance;
    }
    void render() override;
};