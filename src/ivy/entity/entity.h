#ifndef IVY_ENTITY_H
#define IVY_ENTITY_H

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

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> components_;
};

}

#include "entity.inl"

#endif // IVY_ENTITY_H
