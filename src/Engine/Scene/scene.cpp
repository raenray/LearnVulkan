#include "Engine/Scene/scene.hpp"
#include <algorithm>
#include <stdexcept>

namespace Engine
{
namespace Scene
{

// --- SceneNode ---

SceneNode::SceneNode(ECS::World& world, ECS::Entity entity)
    : world_(&world)
    , entity_(entity)
{
}

void SceneNode::setParent(SceneNode* parent)
{
    // Remove from old parent
    if (parent_)
    {
        auto& siblings = parent_->children_;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }
    parent_ = parent;
    if (parent_)
        parent_->children_.push_back(this);
}

void SceneNode::addChild(SceneNode* child)
{
    child->setParent(this);
}

glm::mat4 SceneNode::localMatrix() const
{
    auto* tc = world_->getComponent<ECS::TransformComponent>(entity_);
    return tc ? tc->matrix() : glm::mat4(1.0f);
}

glm::mat4 SceneNode::worldMatrix() const
{
    glm::mat4 local = localMatrix();
    if (parent_)
        return parent_->worldMatrix() * local;
    return local;
}

glm::vec3 SceneNode::position() const
{
    auto* tc = world_->getComponent<ECS::TransformComponent>(entity_);
    return tc ? tc->position : glm::vec3(0.0f);
}

void SceneNode::setPosition(const glm::vec3& pos)
{
    auto* tc = world_->getComponent<ECS::TransformComponent>(entity_);
    if (tc)
        tc->position = pos;
}

glm::quat SceneNode::rotation() const
{
    auto* tc = world_->getComponent<ECS::TransformComponent>(entity_);
    return tc ? tc->rotation : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

void SceneNode::setRotation(const glm::quat& rot)
{
    auto* tc = world_->getComponent<ECS::TransformComponent>(entity_);
    if (tc)
        tc->rotation = rot;
}

glm::vec3 SceneNode::scale() const
{
    auto* tc = world_->getComponent<ECS::TransformComponent>(entity_);
    return tc ? tc->scale : glm::vec3(1.0f);
}

void SceneNode::setScale(const glm::vec3& s)
{
    auto* tc = world_->getComponent<ECS::TransformComponent>(entity_);
    if (tc)
        tc->scale = s;
}

template <typename Func>
void SceneNode::traverse(Func&& func)
{
    func(*this);
    for (auto* child : children_)
        child->traverse(std::forward<Func>(func));
}

// Explicit instantiation is not needed since traverse is template — we keep it
// inline in the header.

// --- Scene ---

Scene::Scene() = default;
Scene::~Scene() = default;

SceneNode* Scene::createNode(const std::string& tag)
{
    ECS::Entity e = world_.createEntity(tag);
    world_.setComponent<ECS::TransformComponent>(e, {});
    auto node = std::make_unique<SceneNode>(world_, e);
    SceneNode* ptr = node.get();
    nodes_.push_back(std::move(node));
    return ptr;
}

void Scene::destroyNode(SceneNode* node)
{
    if (!node)
        return;
    // Detach from parent
    node->setParent(nullptr);
    // Recursively remove children
    for (auto* child : node->children())
        destroyNode(child);
    world_.destroyEntity(node->entity());
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [node](auto& n) { return n.get() == node; });
    if (it != nodes_.end())
        nodes_.erase(it);
}

SceneNode* Scene::findNode(ECS::Entity entity)
{
    for (auto& n : nodes_)
        if (n->entity() == entity)
            return n.get();
    return nullptr;
}

SceneNode* Scene::findPrimaryCamera()
{
    for (auto& n : nodes_)
    {
        auto* cam = world_.getComponent<ECS::CameraComponent>(n->entity());
        if (cam && cam->isPrimary)
            return n.get();
    }
    // Fallback: return first camera
    for (auto& n : nodes_)
    {
        if (world_.hasComponent<ECS::CameraComponent>(n->entity()))
            return n.get();
    }
    return nullptr;
}

std::vector<ECS::Entity> Scene::gatherLights() const
{
    return world_.entitiesWith<ECS::LightComponent>();
}

void Scene::updateTransforms()
{
    // World matrices are computed lazily via worldMatrix().
    // This method exists for future dirty-flag optimizations.
}

} // namespace Scene
} // namespace Engine
