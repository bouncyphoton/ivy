#ifndef IVY_CONSTS_H
#define IVY_CONSTS_H

#include "types.h"

namespace ivy {
namespace consts {

constexpr const char *ENGINE_NAME = "ivy";

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
