#ifndef IVY_CONSTS_H
#define IVY_CONSTS_H

#include "types.h"
#include <vulkan/vulkan.h>

namespace ivy {
namespace consts {

//----------------------------------
// Engine
//----------------------------------

constexpr const char *ENGINE_NAME = "ivy";

//----------------------------------
// Renderer
//----------------------------------

constexpr u32 DESIRED_SWAPCHAIN_IMAGES = 3;
constexpr VkPresentModeKHR DESIRED_PRESENT_MODE = VK_PRESENT_MODE_MAILBOX_KHR;

//----------------------------------
// Development
//----------------------------------

constexpr bool DEBUG =
#if defined(NDEBUG)
    false
#else
    true
#endif
    ;

}
}

#endif //IVY_CONSTS_H
