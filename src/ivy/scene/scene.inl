#include "scene.h"

template <typename ... Components>
EntityHandle Scene::findEntityWithAllComponents() {
    for (EntityHandle entity : *this) {
        if (entity->hasAllComponents<Components...>()) {
            return entity;
        }
    }

    // Otherwise, couldn't find any entities with these components so return invalid entity handle
    return EntityHandle(*this);
}

template<typename... Components>
std::vector<EntityHandle> Scene::findEntitiesWithAllComponents() {
    std::vector<EntityHandle> foundEntities;

    for (EntityHandle entity : *this) {
        if (entity->hasAllComponents<Components...>()) {
            foundEntities.emplace_back(entity);
        }
    }

    return foundEntities;
}

template<typename... Components>
std::vector<EntityHandle> Scene::findEntitiesWithAnyComponents() {
    std::vector<EntityHandle> foundEntities;

    for (EntityHandle entity : *this) {
        if (entity->hasAnyComponents<Components...>()) {
            foundEntities.emplace_back(entity);
        }
    }

    return foundEntities;
}
