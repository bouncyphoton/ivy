#include "entity.h"

namespace ivy {

template<typename T>
T *Entity::getComponent() const {
    auto it = components_.find(typeid(T));
    if (it != components_.end()) {
        return static_cast<T*>(it->second.get());
    }

    return nullptr;
}

template<typename T>
const T *Entity::getComponentConst() const {
    auto it = components_.find(typeid(T));
    if (it != components_.end()) {
        return static_cast<T*>(it->second.get());
    }

    return nullptr;
}

template<typename T>
void Entity::setComponent(const T &component) {
    components_[typeid(T)] = std::make_shared<T>(component);
}

template<typename T>
void Entity::removeComponent() {
    components_.erase(typeid(T));
}

}
