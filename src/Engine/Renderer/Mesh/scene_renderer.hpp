#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Engine
{
namespace Scene
{
class Scene;
}

namespace Renderer
{
class MeshCache;
class DeferredRenderer;

// Reads Scene/ECS data and issues draw commands through the DeferredRenderer.
// Does NOT begin/end render passes — the caller controls pass boundaries.
class SceneRenderer
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;

    // Issue draw calls for every entity that has MeshComponent + TransformComponent.
    // Call between vkCmdBeginRenderPass(geoRP) and vkCmdEndRenderPass.
    void drawGeometryPass(VkCommandBuffer cb,
                          Scene::Scene& scene,
                          MeshCache& meshCache,
                          DeferredRenderer& def,
                          VkExtent2D extent,
                          glm::mat4 view,
                          glm::mat4 proj,
                          uint32_t& outDrawCalls);

    // Issue the lighting pass full-screen draw call.
    // Call between vkCmdBeginRenderPass(swapchainRP) and vkCmdEndRenderPass.
    void drawLightingPass(VkCommandBuffer cb, Scene::Scene& scene, DeferredRenderer& def, glm::vec3 cameraPos);
};
} // namespace Renderer
} // namespace Engine
