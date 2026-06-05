#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
struct GLFWwindow;

namespace Engine
{
namespace Scene
{
class Scene;
class SceneNode;
} // namespace Scene

namespace Renderer
{
class MeshCache;
}

// User-facing gameplay class — create entities here, update per-frame logic here.
// The engine (App) calls init() once and update() every frame.
class Game
{
public:
    Game() = default;
    ~Game() = default;

    // Create scene entities: camera, light, meshes, etc.
    void init(Scene::Scene& scene, Renderer::MeshCache& meshCache);

    // Per-frame gameplay. Input is polled from the GLFW window.
    void update(float dt, GLFWwindow* window);

    // Camera state for the renderer
    Scene::SceneNode* cameraNode() const { return cameraNode_; }

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;
    glm::vec3 cameraPosition() const;
    glm::vec3 cameraForward() const;

    float cameraFov() const { return 60.0f; }

    // Player-input camera control
    float cameraYaw = -90.0f;
    float cameraPitch = -15.0f;
    int lastMouseX = 0;
    int lastMouseY = 0;

private:
    Scene::Scene* scene_ = nullptr;
    Scene::SceneNode* cameraNode_ = nullptr;
    Scene::SceneNode* sphereNode_ = nullptr;
    Scene::SceneNode* lightNode_ = nullptr;
    float sphereAngle_ = 0.0f;
};
} // namespace Engine
