#ifndef IVY_ENTITY_H
#define IVY_ENTITY_H

#include "ivy/entity/components/component.h"
#include <unordered_map>
#include <memory>
#include <typeindex>

namespace ivy {

class Entity {
public:

    /**
     * \brief Search for and get a certain component from the entity
     * \tparam T The component type
     * \return A pointer to the component if found, otherwise nullptr
     */
    template <typename T>
    T *getComponent() const;

    /**
     * \brief Search for and get a certain component from the entity
     * \tparam T The component type
     * \return A const pointer to the component if found, otherwise nullptr
     */
    template <typename T>
    const T *getComponentConst() const;

    /**
     * \brief Set a component for the entity
     * \tparam T The component type
     * \param component The data to set for the component
     */
    template <typename T>
    void setComponent(const T &component = T{});

    /**
     * \brief Remove a component from the entity
     * \tparam T The component type
     */
    template <typename T>
    void removeComponent();

    void setTag(const std::string &tag) {
        tag_ = tag;
    }

    bool hasTag(const std::string &tag) const {
        return tag_ == tag;
    }

private:
    std::string tag_;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> components_;
};

}

#include "entity.inl"

#endif // IVY_ENTITY_H
