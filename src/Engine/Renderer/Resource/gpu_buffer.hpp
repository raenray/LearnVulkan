#pragma once
#include <vulkan/vulkan.h>
#include <cstddef>

namespace Engine
{
namespace Renderer
{
struct GPUBufferCreateInfo
{
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
    VkMemoryPropertyFlags requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryPropertyFlags preferredFlags = 0;
    const char* name = nullptr;
};

class GPUBuffer
{
public:
    GPUBuffer(void* vma, const GPUBufferCreateInfo& ci);
    ~GPUBuffer();
    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;
    GPUBuffer(GPUBuffer&&) noexcept;
    GPUBuffer& operator=(GPUBuffer&&) noexcept;

    VkBuffer buffer() const { return buf_; }

    VkDeviceSize size() const { return sz_; }

    void* map();
    void unmap();
    void flush(VkDeviceSize o = 0, VkDeviceSize s = VK_WHOLE_SIZE);
    void invalidate(VkDeviceSize o = 0, VkDeviceSize s = VK_WHOLE_SIZE);

    bool isMapped() const { return mapped_ != nullptr; }

    void upload(VkCommandBuffer cmd, VkQueue q, const void* data, VkDeviceSize s, VkDeviceSize dstOff = 0);

private:
    VkDevice dev_ = VK_NULL_HANDLE;
    void* allocator_ = nullptr;
    VkBuffer buf_ = VK_NULL_HANDLE;
    void* allocation_ = nullptr;
    VkDeviceSize sz_ = 0;
    void* mapped_ = nullptr;
};

typedef GPUBuffer GpuBuffer;
typedef GPUBufferCreateInfo GpuBufferDesc;
} // namespace Renderer
} // namespace Engine
