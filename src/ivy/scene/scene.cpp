#include "scene.h"
#include "ivy/log.h"

namespace ivy {

EntityHandle::EntityHandle(Scene &scene)
    : scene_(scene), entityIdx_(-1), version_(Scene::VERSION_INVALID_BIT) {}

EntityHandle::EntityHandle(Scene &scene, u32 entity_idx, u32 version)
    : scene_(scene), entityIdx_(entity_idx), version_(version) {}

EntityHandle &EntityHandle::operator=(const EntityHandle &e) {
    if (this == &e) {
        return *this;
    }

    if (&scene_ != &e.scene_) {
        Log::fatal("Cannot copy entity handle across scenes");
    }

    entityIdx_ = e.entityIdx_;
    version_ = e.version_;
    return *this;
}

EntityHandle::operator bool() {
    return *(*this);
}

Entity *EntityHandle::operator*() {
    // Make sure we have a valid index
    if (entityIdx_ >= scene_.entities_.size() ||
        entityIdx_ >= scene_.versions_.size()) {
        // entities_.size() should equal version_.size(), so second part of if is redundant
        return nullptr;
    }

    if ((version_ & Scene::VERSION_INVALID_BIT) ||
        version_ != scene_.versions_.at(entityIdx_)) {
        // The entity we're pointing to is out of date or deleted
        return nullptr;
    }

    return &scene_.entities_.at(entityIdx_);
}

Entity *EntityHandle::operator->() {
    return *(*this);
}

SceneIterator &SceneIterator::operator++() {
    do {
        ++entityIdx_;
    } while (entityIdx_ < scene_.entities_.size() &&
             (scene_.versions_.at(entityIdx_) & Scene::VERSION_INVALID_BIT));

    return *this;
}

EntityHandle SceneIterator::operator*() {
    return EntityHandle(scene_, entityIdx_, scene_.versions_.at(entityIdx_));
}

EntityHandle Scene::createEntity() {
    u32 idx;
    u32 version;
    if (!deletedIndices_.empty()) {
        // Re-use the memory of a deleted entity
        idx = deletedIndices_.back();
        deletedIndices_.pop_back();
        versions_[idx] = (versions_[idx] & ~VERSION_INVALID_BIT) + 1;
        version = versions_[idx];
    } else {
        idx = entities_.size();
        version = 0;

        entities_.emplace_back();
        versions_.emplace_back();
    }

    return EntityHandle(*this, idx, version);
}

void Scene::deleteEntity(EntityHandle entity) {
    if (entity) {
        deletedIndices_.emplace_back(entity.entityIdx_);
        versions_[entity.entityIdx_] |= VERSION_INVALID_BIT;
        entities_[entity.entityIdx_] = Entity();
    }
}

std::vector<EntityHandle> Scene::findEntitiesWithTag(const std::string &tag) {
    std::vector<EntityHandle> foundEntities;

    for (EntityHandle entity : *this) {
        if (entity->hasTag(tag)) {
            foundEntities.emplace_back(entity);
        }
    }

    return foundEntities;
}

SceneIterator Scene::begin() {
    SceneIterator it(*this, 0);

    // If first entity is not valid, go to next valid or end
    if (!*it) {
        ++it;
    }

    return it;
}

SceneIterator Scene::end() {
    return SceneIterator(*this, entities_.size());
}

size_t Scene::size() {
    return entities_.size() - deletedIndices_.size();
}

size_t Scene::capacity() {
    return entities_.size();
}

bool Scene::empty() {
    return size() == 0;
}

}
