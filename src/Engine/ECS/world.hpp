#pragma once
#include "Engine/ECS/components.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include <memory>

namespace Engine
{
namespace ECS
{

// Simple archetype-based ECS world.
// Uses a component-centric approach: each component type is stored in its own
// dense array, indexed by entity ID. Entity IDs are simple uint32_t.

using Entity = uint32_t;
constexpr Entity INVALID_ENTITY = ~0u;

class World
{
public:
    World() = default;
    ~World() = default;

    // Entity management
    Entity createEntity(const std::string& tag = "");
    void destroyEntity(Entity e);
    bool entityValid(Entity e) const;

    // Component setters/getters (simple per-component maps)
    template <typename T>
    void setComponent(Entity e, const T& component);

    template <typename T>
    T* getComponent(Entity e);

    template <typename T>
    const T* getComponent(Entity e) const;

    template <typename T>
    void removeComponent(Entity e);

    template <typename T>
    bool hasComponent(Entity e) const;

    // Iteration helpers
    template <typename T>
    std::vector<Entity> entitiesWith() const;

    // Get all entities
    const std::vector<Entity>& entities() const { return entities_; }

    // Clear everything
    void clear();

private:
    std::vector<Entity> entities_;
    uint32_t nextEntity_ = 0;

    // Component storage: one vector per component type
    // We use type-erased storage for simplicity
    struct ComponentStorageBase
    {
        virtual ~ComponentStorageBase() = default;
        virtual void remove(Entity e) = 0;
        virtual bool has(Entity e) const = 0;
    };

    template <typename T>
    struct ComponentStorage : ComponentStorageBase
    {
        std::unordered_map<Entity, T> data;

        void remove(Entity e) override { data.erase(e); }

        bool has(Entity e) const override { return data.find(e) != data.end(); }
    };

    // Map from type hash to storage
    std::unordered_map<size_t, std::unique_ptr<ComponentStorageBase>> storages_;

    template <typename T>
    static size_t typeId();

    template <typename T>
    ComponentStorage<T>& storage();
    template <typename T>
    const ComponentStorage<T>& storage() const;
};

// --- Template implementations ---

template <typename T>
size_t World::typeId()
{
    return typeid(T).hash_code();
}

template <typename T>
World::ComponentStorage<T>& World::storage()
{
    size_t id = typeId<T>();
    auto it = storages_.find(id);
    if (it == storages_.end())
    {
        storages_[id] = std::make_unique<ComponentStorage<T>>();
        return *static_cast<ComponentStorage<T>*>(storages_[id].get());
    }
    return *static_cast<ComponentStorage<T>*>(it->second.get());
}

template <typename T>
const World::ComponentStorage<T>& World::storage() const
{
    size_t id = typeId<T>();
    auto it = storages_.find(id);
    static ComponentStorage<T> empty;
    if (it == storages_.end())
        return empty;
    return *static_cast<const ComponentStorage<T>*>(it->second.get());
}

template <typename T>
void World::setComponent(Entity e, const T& component)
{
    storage<T>().data[e] = component;
}

template <typename T>
T* World::getComponent(Entity e)
{
    auto& s = storage<T>();
    auto it = s.data.find(e);
    return (it != s.data.end()) ? &it->second : nullptr;
}

template <typename T>
const T* World::getComponent(Entity e) const
{
    auto& s = storage<T>();
    auto it = s.data.find(e);
    return (it != s.data.end()) ? &it->second : nullptr;
}

template <typename T>
void World::removeComponent(Entity e)
{
    storage<T>().remove(e);
}

template <typename T>
bool World::hasComponent(Entity e) const
{
    return storage<T>().has(e);
}

template <typename T>
std::vector<Entity> World::entitiesWith() const
{
    std::vector<Entity> result;
    auto& s = storage<T>();
    for (auto& [e, _] : s.data)
        if (entityValid(e))
            result.push_back(e);
    return result;
}

} // namespace ECS
} // namespace Engine
