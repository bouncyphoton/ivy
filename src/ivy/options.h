#ifndef IVY_OPTIONS_H
#define IVY_OPTIONS_H

#include "ivy/types.h"

namespace ivy {

/**
 * \brief Engine options
 */
struct Options {
    u32 renderWidth = 1280;
    u32 renderHeight = 720;
    u32 numFramesInFlight = 3;

    enum class PresentModeEnum {
        IMMEDIATE, MAILBOX, FIFO
    } desiredPresentMode = PresentModeEnum::MAILBOX;

    const char *appName = "ivy_app";
};

}

#endif //IVY_OPTIONS_H
