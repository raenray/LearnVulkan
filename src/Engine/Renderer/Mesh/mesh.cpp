#include "Engine/Renderer/Mesh/mesh.hpp"
#include "Engine/Renderer/Resource/gpu_buffer.hpp"

namespace Engine
{
namespace Renderer
{

Mesh::Mesh(std::unique_ptr<GpuBuffer> vb, std::unique_ptr<GpuBuffer> ib, uint32_t vertexCount, uint32_t indexCount)
    : vb_(std::move(vb))
    , ib_(std::move(ib))
    , vertexCount_(vertexCount)
    , indexCount_(indexCount)
{
}

Mesh::~Mesh() = default;

} // namespace Renderer
} // namespace Engine
