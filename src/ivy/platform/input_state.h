#ifndef IVY_INPUT_STATE_H
#define IVY_INPUT_STATE_H

#include "ivy/types.h"
#include <unordered_map>

namespace ivy {

class InputState {
public:

    bool isKeyDown(u32 key) const;

    bool isKeyPressed(u32 key) const;

    void transitionKeyStates();

    void pressKey(u32 key);

    void releaseKey(u32 key);

private:
    enum class KeyState {
        UP, DOWN, PRESSED
    };

    std::unordered_map<u32, KeyState> keyStates_;
};

}

#endif // IVY_INPUT_STATE_H
