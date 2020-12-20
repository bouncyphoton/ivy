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
