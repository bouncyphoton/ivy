#ifndef IVY_COMPONENT_H
#define IVY_COMPONENT_H

#include <string>

namespace ivy {

/**
 * \brief Component base class
 */
class Component {
public:
    virtual ~Component() = default;

    [[nodiscard]] virtual std::string getName() const = 0;
};

}

#endif //IVY_COMPONENT_H
