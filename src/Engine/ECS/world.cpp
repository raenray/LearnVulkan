#include "Engine/ECS/world.hpp"
#include <typeinfo>
#include <stdexcept>
#include <cstring>

namespace Engine
{
namespace ECS
{

Entity World::createEntity(const std::string& tag)
{
    Entity e = nextEntity_++;
    entities_.push_back(e);
    if (!tag.empty())
    {
        TagComponent tc;
        strncpy(tc.tag, tag.c_str(), sizeof(tc.tag) - 1);
        setComponent<TagComponent>(e, tc);
    }
    return e;
}

void World::destroyEntity(Entity e)
{
    // Remove from entities list
    auto it = std::find(entities_.begin(), entities_.end(), e);
    if (it != entities_.end())
        entities_.erase(it);

    // Remove all components
    for (auto& [_, storage] : storages_)
        storage->remove(e);
}

bool World::entityValid(Entity e) const
{
    return std::find(entities_.begin(), entities_.end(), e) != entities_.end();
}

void World::clear()
{
    entities_.clear();
    storages_.clear();
    nextEntity_ = 0;
}

} // namespace ECS
} // namespace Engine
