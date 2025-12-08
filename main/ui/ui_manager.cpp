#include "ui_manager.hpp"
#include "screen.hpp"

void UIManager::setScreen(Screen* s) {
    current = s;
    render();
}

void UIManager::render() {
    if (current) current->render();
}
