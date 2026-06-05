#pragma once
#include "Engine/ECS/world.hpp"
#include "Engine/ECS/components.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <memory>

namespace Engine
{
namespace Scene
{

// High-level scene graph node. Wraps an ECS entity and provides a hierarchical
// transform system with parent-child relationships.

class SceneNode
{
public:
    SceneNode(ECS::World& world, ECS::Entity entity);
    ~SceneNode() = default;

    ECS::Entity entity() const { return entity_; }

    // Hierarchy
    void setParent(SceneNode* parent);

    SceneNode* parent() const { return parent_; }

    const std::vector<SceneNode*>& children() const { return children_; }

    void addChild(SceneNode* child);

    // Transform helpers
    glm::mat4 localMatrix() const;
    glm::mat4 worldMatrix() const;
    glm::vec3 position() const;
    void setPosition(const glm::vec3& pos);
    glm::quat rotation() const;
    void setRotation(const glm::quat& rot);
    glm::vec3 scale() const;
    void setScale(const glm::vec3& s);

    // Utility: iterate subtree
    template <typename Func>
    void traverse(Func&& func); // func(SceneNode&)

private:
    ECS::World* world_ = nullptr;
    ECS::Entity entity_ = ECS::INVALID_ENTITY;
    SceneNode* parent_ = nullptr;
    std::vector<SceneNode*> children_;
};

// Scene manages a collection of scene nodes backed by an ECS World
class Scene
{
public:
    Scene();
    ~Scene();

    ECS::World& world() { return world_; }

    const ECS::World& world() const { return world_; }

    SceneNode* createNode(const std::string& tag = "");
    void destroyNode(SceneNode* node);
    SceneNode* findNode(ECS::Entity entity);

    // Find the primary camera (first CameraComponent with isPrimary=true)
    SceneNode* findPrimaryCamera();

    // Gather all light entities
    std::vector<ECS::Entity> gatherLights() const;

    // Update transforms (propagate world matrices)
    void updateTransforms();

private:
    ECS::World world_;
    std::vector<std::unique_ptr<SceneNode>> nodes_;
};

} // namespace Scene
} // namespace Engine
