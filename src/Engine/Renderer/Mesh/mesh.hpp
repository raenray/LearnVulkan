#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include "Engine/Renderer/Resource/gpu_buffer.hpp"

namespace Engine
{
namespace Renderer
{

// Vertex layout for the deferred geometry pass (matches gbuffer.vert).
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
};

// GPU-resident triangle mesh: vertex + index buffers with a count.
class Mesh
{
public:
    Mesh(std::unique_ptr<GpuBuffer> vb, std::unique_ptr<GpuBuffer> ib, uint32_t vertexCount, uint32_t indexCount);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    VkBuffer vertexBuffer() const { return vb_->buffer(); }

    VkBuffer indexBuffer() const { return ib_->buffer(); }

    uint32_t vertexCount() const { return vertexCount_; }

    uint32_t indexCount() const { return indexCount_; }

private:
    std::unique_ptr<GpuBuffer> vb_;
    std::unique_ptr<GpuBuffer> ib_;
    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;
};
} // namespace Renderer
} // namespace Engine
