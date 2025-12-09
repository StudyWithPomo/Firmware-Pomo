#pragma once

class UIManager;

class Screen {
public:
    virtual void render() = 0;     // pure virtual
    virtual ~Screen() = default;   // inline default destructor
};
