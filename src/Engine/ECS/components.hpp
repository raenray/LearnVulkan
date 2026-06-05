#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <cstdint>

namespace Engine
{
namespace ECS
{

// --- Tag components ---

struct TagComponent
{
    std::string tag;
};

// --- Transform ---

struct TransformComponent
{
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // w,x,y,z
    glm::vec3 scale{1.0f};

    glm::mat4 matrix() const
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
        m = m * glm::mat4_cast(rotation);
        m = glm::scale(m, scale);
        return m;
    }
};

// --- Mesh reference ---

struct MeshComponent
{
    std::string meshId;     // Asset identifier
    std::string materialId; // Material identifier
    bool visible = true;
    bool castShadow = true;
};

// --- Camera ---

struct CameraComponent
{
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    bool isPrimary = false;

    glm::mat4 projection(float aspectRatio) const
    {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    glm::mat4 view(const TransformComponent& transform) const { return glm::inverse(transform.matrix()); }
};

// --- Light types ---

enum class LightType : uint8_t
{
    Directional = 0,
    Point = 1,
    Spot = 2
};

struct LightComponent
{
    LightType type = LightType::Directional;
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;          // point/spot
    float innerConeAngle = 12.0f; // spot
    float outerConeAngle = 20.0f; // spot
};

// --- PBR Material ---

struct MaterialComponent
{
    std::string albedoTexture;
    std::string normalTexture;
    std::string metallicRoughnessTexture;
    std::string aoTexture;
    glm::vec4 albedoFactor{1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float aoFactor = 1.0f;
};

// --- Velocity / physics ---

struct VelocityComponent
{
    glm::vec3 linear{0.0f};
    glm::vec3 angular{0.0f};
};

} // namespace ECS
} // namespace Engine
