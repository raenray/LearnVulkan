#include "Engine/Game/game.hpp"
#include "Engine/Scene/scene.hpp"
#include "Engine/ECS/world.hpp"
#include "Engine/ECS/components.hpp"
#include "Engine/Renderer/Mesh/mesh_cache.hpp"
#include "Engine/Renderer/Mesh/mesh.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace Engine
{

void Game::init(Scene::Scene& scene, Renderer::MeshCache& meshCache)
{
    scene_ = &scene;

    // ── Camera ───────────────────────────────────────────────────────
    cameraNode_ = scene.createNode("MainCamera");
    cameraNode_->setPosition({0.0f, 2.5f, 6.0f});
    {
        ECS::CameraComponent cam;
        cam.fov = 60.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 100.0f;
        cam.isPrimary = true;
        scene.world().setComponent<ECS::CameraComponent>(cameraNode_->entity(), cam);
    }

    // ── Directional light ────────────────────────────────────────────
    lightNode_ = scene.createNode("Sun");
    lightNode_->setPosition({0.0f, 10.0f, 0.0f});
    lightNode_->setRotation(glm::angleAxis(glm::radians(30.0f), glm::vec3(1, 0, 0)));
    {
        ECS::LightComponent lc;
        lc.type = ECS::LightType::Directional;
        lc.color = glm::vec3(1.0f, 0.95f, 0.85f);
        lc.intensity = 1.5f;
        scene.world().setComponent<ECS::LightComponent>(lightNode_->entity(), lc);
    }

    // ── Sphere ───────────────────────────────────────────────────────
    sphereNode_ = scene.createNode("Sphere");
    sphereNode_->setPosition({0.0f, 2.5f, 0.0f});
    {
        ECS::MeshComponent mc;
        strncpy(mc.meshId, "sphere", sizeof(mc.meshId));
        strncpy(mc.materialId, "default", sizeof(mc.materialId));
        mc.visible = true;
        mc.castShadow = true;
        scene.world().setComponent<ECS::MeshComponent>(sphereNode_->entity(), mc);
    }
    {
        ECS::MaterialComponent mat;
        mat.albedoFactor = glm::vec4(0.8f, 0.3f, 0.1f, 1.0f);
        mat.metallicFactor = 0.0f;
        mat.roughnessFactor = 0.4f;
        mat.aoFactor = 1.0f;
        scene.world().setComponent<ECS::MaterialComponent>(sphereNode_->entity(), mat);
    }
}

void Game::update(float dt, GLFWwindow* window)
{
    if (!scene_)
        return;

    // ── Camera movement ──────────────────────────────────────────────
    float speed = 5.0f * dt;
    float ry = glm::radians(cameraYaw), rp = glm::radians(cameraPitch);
    glm::vec3 fwd{cos(ry) * cos(rp), sin(rp), sin(ry) * cos(rp)};
    glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));
    glm::vec3 pos = cameraNode_->position();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pos += fwd * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pos -= fwd * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        pos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        pos += right * speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        pos.y -= speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        pos.y += speed;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        pos = {0.0f, 2.5f, 6.0f};
        cameraYaw = -90.0f;
        cameraPitch = -15.0f;
    }

    cameraNode_->setPosition(pos);

    // ── Rotate sphere ────────────────────────────────────────────────
    if (sphereNode_)
    {
        sphereAngle_ += 0.8f * dt;
        sphereNode_->setRotation(glm::angleAxis(sphereAngle_, glm::vec3(0, 1, 0)));
    }
}

glm::mat4 Game::viewMatrix() const
{
    glm::vec3 pos = cameraNode_ ? cameraNode_->position() : glm::vec3(0, 2.5f, 6);
    glm::vec3 fwd = cameraForward();
    return glm::lookAt(pos, pos + fwd, glm::vec3(0, 1, 0));
}

glm::mat4 Game::projectionMatrix(float aspect) const
{
    glm::mat4 proj = glm::perspective(glm::radians(cameraFov()), aspect, 0.1f, 100.0f);
    proj[1][1] *= -1; // flip Y for Vulkan
    return proj;
}

glm::vec3 Game::cameraPosition() const
{
    return cameraNode_ ? cameraNode_->position() : glm::vec3(0, 2.5f, 6);
}

glm::vec3 Game::cameraForward() const
{
    float ry = glm::radians(cameraYaw), rp = glm::radians(cameraPitch);
    return glm::vec3(cos(ry) * cos(rp), sin(rp), sin(ry) * cos(rp));
}

} // namespace Engine
