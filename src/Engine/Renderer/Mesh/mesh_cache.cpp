#include "Engine/Renderer/Mesh/mesh_cache.hpp"
#include "Engine/Renderer/Resource/gpu_buffer.hpp"
#include <cstring>

namespace Engine
{
namespace Renderer
{

void MeshCache::createMesh(const std::string& id,
                           const std::vector<Vertex>& vertices,
                           const std::vector<uint32_t>& indices,
                           void* vmaAllocator,
                           VkDevice device,
                           VkCommandPool cmdPool,
                           VkQueue queue)
{
    VkDeviceSize vsz = sizeof(Vertex) * vertices.size();
    VkDeviceSize isz = sizeof(uint32_t) * indices.size();

    // Staging buffer for vertices
    GpuBufferDesc vbStg{};
    vbStg.size = vsz;
    vbStg.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vbStg.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    GpuBuffer stgVb(vmaAllocator, vbStg);
    memcpy(stgVb.map(), vertices.data(), (size_t) vsz);

    // Device-local vertex buffer
    GpuBufferDesc vbDesc{};
    vbDesc.size = vsz;
    vbDesc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    auto vb = std::make_unique<GpuBuffer>(vmaAllocator, vbDesc);

    // Staging buffer for indices
    GpuBufferDesc ibStg{};
    ibStg.size = isz;
    ibStg.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    ibStg.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    GpuBuffer stgIb(vmaAllocator, ibStg);
    memcpy(stgIb.map(), indices.data(), (size_t) isz);

    // Device-local index buffer
    GpuBufferDesc ibDesc{};
    ibDesc.size = isz;
    ibDesc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    auto ib = std::make_unique<GpuBuffer>(vmaAllocator, ibDesc);

    // One-time submit to copy staging → device-local
    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = cmdPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cb;
    vkAllocateCommandBuffers(device, &ai, &cb);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);

    VkBufferCopy vc{0, 0, vsz};
    vkCmdCopyBuffer(cb, stgVb.buffer(), vb->buffer(), 1, &vc);
    VkBufferCopy ic{0, 0, isz};
    vkCmdCopyBuffer(cb, stgIb.buffer(), ib->buffer(), 1, &ic);

    vkEndCommandBuffer(cb);

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, cmdPool, 1, &cb);

    meshes_[id] = std::make_unique<Mesh>(std::move(vb), std::move(ib), (uint32_t) vertices.size(), (uint32_t) indices.size());
}

Mesh* MeshCache::get(const std::string& id)
{
    auto it = meshes_.find(id);
    return it != meshes_.end() ? it->second.get() : nullptr;
}

bool MeshCache::has(const std::string& id) const
{
    return meshes_.find(id) != meshes_.end();
}

void MeshCache::clear()
{
    meshes_.clear();
}

} // namespace Renderer
} // namespace Engine
