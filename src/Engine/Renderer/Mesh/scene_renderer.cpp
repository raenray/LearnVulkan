#include "Engine/Renderer/Mesh/scene_renderer.hpp"
#include "Engine/Renderer/Mesh/mesh_cache.hpp"
#include "Engine/Renderer/Vulkan/deferred_renderer.hpp"
#include "Engine/Scene/scene.hpp"
#include "Engine/ECS/world.hpp"
#include "Engine/ECS/components.hpp"

namespace Engine
{
namespace Renderer
{

void SceneRenderer::drawGeometryPass(VkCommandBuffer cb,
                                     Scene::Scene& scene,
                                     MeshCache& meshCache,
                                     DeferredRenderer& def,
                                     VkExtent2D extent,
                                     glm::mat4 view,
                                     glm::mat4 proj,
                                     uint32_t& outDrawCalls)
{
    outDrawCalls = 0;

    // Bind pipeline & viewport once
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, def.geoPipe());
    VkViewport vp{0, (float) extent.height, (float) extent.width, -(float) extent.height, 0, 1};
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D sc{{0, 0}, extent};
    vkCmdSetScissor(cb, 0, 1, &sc);

    // Iterate every entity that has both a mesh and a transform
    auto meshEntities = scene.world().entitiesWith<ECS::MeshComponent>();

    for (auto e : meshEntities)
    {
        auto* transform = scene.world().getComponent<ECS::TransformComponent>(e);
        auto* meshComp = scene.world().getComponent<ECS::MeshComponent>(e);
        if (!transform || !meshComp || !meshComp->visible)
            continue;

        Mesh* mesh = meshCache.get(meshComp->meshId);
        if (!mesh)
            continue;

        // Compute world matrix through the scene node hierarchy
        auto* node = scene.findNode(e);
        glm::mat4 model = node ? node->worldMatrix() : transform->matrix();
        glm::mat4 mvp = proj * view * model;

        // Push constants (matches gbuffer.vert layout)
        struct
        {
            glm::mat4 mvp;
            glm::mat4 mdl;
        } pc;

        pc.mvp = mvp;
        pc.mdl = model;
        vkCmdPushConstants(cb, def.geoLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

        // Bind mesh buffers
        VkBuffer vbo[] = {mesh->vertexBuffer()};
        VkDeviceSize off[] = {0};
        vkCmdBindVertexBuffers(cb, 0, 1, vbo, off);
        vkCmdBindIndexBuffer(cb, mesh->indexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cb, mesh->indexCount(), 1, 0, 0, 0);
        ++outDrawCalls;
    }
}

void SceneRenderer::drawLightingPass(VkCommandBuffer cb, Scene::Scene& scene, DeferredRenderer& def, glm::vec3 cameraPos)
{
    // Gather directional light (use first directional light found)
    auto lightEntities = scene.world().entitiesWith<ECS::LightComponent>();
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, 0.3f));
    glm::vec4 lightColor(1.0f, 0.95f, 0.85f, 1.5f);

    for (auto e : lightEntities)
    {
        auto* lc = scene.world().getComponent<ECS::LightComponent>(e);
        if (lc && lc->type == ECS::LightType::Directional)
        {
            // Direction from the light entity's forward vector
            auto* node = scene.findNode(e);
            // For directional lights, world matrix forward = light direction
            glm::vec3 dir{0, -1, 0}; // default down
            if (node)
            {
                glm::mat4 wm = node->worldMatrix();
                dir = glm::normalize(glm::vec3(wm[2])); // Z axis
            }
            lightDir = glm::normalize(dir);
            lightColor = glm::vec4(lc->color * lc->intensity, lc->intensity);
            break;
        }
    }

    struct
    {
        glm::vec4 cp;
        glm::vec4 ld;
        glm::vec4 lcl;
        glm::mat4 snull;
    } pc;

    pc.cp = glm::vec4(cameraPos, 0);
    pc.ld = glm::vec4(lightDir, 0);
    pc.lcl = lightColor;
    pc.snull = glm::mat4(1);

    vkCmdPushConstants(cb, def.lightLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
    vkCmdDraw(cb, 3, 1, 0, 0);
}

} // namespace Renderer
} // namespace Engine
