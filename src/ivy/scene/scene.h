#ifndef IVY_SCENE_H
#define IVY_SCENE_H

#include "ivy/types.h"
#include "ivy/scene/entity.h"
#include <vector>

namespace ivy {

class Scene;

class EntityHandle {
public:
    /**
     * \brief Construct an invalid entity handle
     * \param scene Parent scene
     */
    explicit EntityHandle(Scene &scene);

    /**
     * \brief Construct an entity handle for an entity
     * \param scene Parent scene
     * \param entity_idx Entity index
     * \param version Current entity version
     */
    EntityHandle(Scene &scene, u32 entity_idx, u32 version);

    EntityHandle(const EntityHandle &e) = default;

    EntityHandle &operator=(const EntityHandle &e);

    /**
     * \return Whether or not the handle is valid
     */
    operator bool();

    Entity *operator *();

    Entity *operator ->();

private:
    friend class Scene;

    Scene &scene_;
    u32 entityIdx_;
    u32 version_;
};

class SceneIterator {
public:
    SceneIterator(Scene &scene, u32 entity_idx)
        : scene_(scene), entityIdx_(entity_idx) {}

    bool operator==(const SceneIterator &rhs) const {
        return &scene_ == &rhs.scene_ && entityIdx_ == rhs.entityIdx_;
    }

    bool operator!=(const SceneIterator &rhs) const {
        return !(rhs == *this);
    }

    SceneIterator &operator++();

    EntityHandle operator*();

private:
    Scene &scene_;
    u32 entityIdx_;
};

/**
 * \brief Provides an interface to interact with entities in a scene
 */
class Scene {
public:
    /**
     * \brief Create an entity, this might invalidate any scene iterators
     * \return A handle to the newly added entity
     */
    EntityHandle createEntity();

    /**
     * \brief Delete an entity from the scene, this does not invalidate scene iterators
     * \param entity The handle to the entity to delete
     */
    void deleteEntity(EntityHandle entity);

    /**
     * \brief Find an entity with the given components
     * \tparam Components The components the entity must have
     * \return An entity handle with the given components or an invalid entity handle if not found
     */
    template <typename... Components>
    [[nodiscard]] EntityHandle findEntityWithAllComponents();

    /**
     * \brief Get a vector of entity handles with entities that have the given components
     * \tparam Components The components the entities must have
     * \return A vector of entity handles
     */
    template <typename... Components>
    [[nodiscard]] std::vector<EntityHandle> findEntitiesWithAllComponents();

    /**
     * \brief Get a vector of entity handles with entities that have the given components
     * \tparam Components The components the entities must have
     * \return A vector of entity handles
     */
    template <typename... Components>
    [[nodiscard]] std::vector<EntityHandle> findEntitiesWithAnyComponents();

    /**
     * \brief Find the entities with a given tag
     * \param tag The tag the entities must have
     * \return A vector of entity handles
     */
    [[nodiscard]] std::vector<EntityHandle> findEntitiesWithTag(const std::string &tag);

    [[nodiscard]] SceneIterator begin();
    [[nodiscard]] SceneIterator end();
    [[nodiscard]] size_t size();
    [[nodiscard]] size_t capacity();
    [[nodiscard]] bool empty();

    /**
     * \brief If this bit is set, then the version is invalid
     */
    static constexpr u32 VERSION_INVALID_BIT = 1 << 31;

private:
    friend class EntityHandle;
    friend class SceneIterator;

    std::vector<Entity> entities_;
    std::vector<u32> versions_;
    std::vector<u32> deletedIndices_;
};

#include "scene.inl"

}

#endif // IVY_SCENE_H
