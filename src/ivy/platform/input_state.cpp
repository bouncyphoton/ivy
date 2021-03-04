#include "input_state.h"

namespace ivy {

bool InputState::isKeyDown(u32 key) const {
    auto it = keyStates_.find(key);
    return it != keyStates_.end() && it->second != KeyState::UP;
}

bool InputState::isKeyPressed(u32 key) const {
    auto it = keyStates_.find(key);
    return it != keyStates_.end() && it->second == KeyState::PRESSED;
}

void InputState::transitionKeyStates() {
    for (auto &pair : keyStates_) {
        if (pair.second == KeyState::PRESSED) {
            pair.second = KeyState::DOWN;
        }
    }
}

void InputState::pressKey(u32 key) {
    if (keyStates_[key] == KeyState::UP) {
        keyStates_[key] = KeyState::PRESSED;
    }
}

void InputState::releaseKey(u32 key) {
    keyStates_[key] = KeyState::UP;
}

}
