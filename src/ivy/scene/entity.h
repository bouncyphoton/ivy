#ifndef IVY_ENTITY_H
#define IVY_ENTITY_H

#include "ivy/scene/components/component.h"
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <ostream>

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

    /**
     * \brief Check if this entity has all the components in the parameter pack
     * \tparam Components Parameter pack of components
     * \return Whether or not all of them are present
     */
    template <typename... Components>
    bool hasAllComponents() const;

    /**
     * \brief Check if this entity has at least one of the components in the parameter pack
     * \tparam Components Parameter pack of components
     * \return Whether or not at least one of them is present
     */
    template <typename... Components>
    bool hasAnyComponents() const;

    void setTag(const std::string &tag) {
        tag_ = tag;
    }

    bool hasTag(const std::string &tag) const {
        return tag_ == tag;
    }

    friend std::ostream &operator<<(std::ostream &os, const Entity &entity);

private:
    std::string tag_;
    std::unordered_map<std::type_index, std::shared_ptr<Component>> components_;
};

}

#include "entity.inl"

#endif // IVY_ENTITY_H
