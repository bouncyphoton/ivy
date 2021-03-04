#include "entity.h"

namespace ivy {

template<typename T>
T *Entity::getComponent() const {
    static_assert(std::is_base_of<Component, T>(), "T is not a valid component");

    auto it = components_.find(typeid(T));
    if (it != components_.end()) {
        return static_cast<T*>(it->second.get());
    }

    return nullptr;
}

template<typename T>
const T *Entity::getComponentConst() const {
    static_assert(std::is_base_of<Component, T>(), "T is not a valid component");

    auto it = components_.find(typeid(T));
    if (it != components_.end()) {
        return static_cast<T*>(it->second.get());
    }

    return nullptr;
}

template<typename T>
void Entity::setComponent(const T &component) {
    static_assert(std::is_base_of<Component, T>(), "T is not a valid component");

    components_[typeid(T)] = std::make_shared<T>(component);
}

template<typename T>
void Entity::removeComponent() {
    static_assert(std::is_base_of<Component, T>(), "T is not a valid component");

    components_.erase(typeid(T));
}

}
