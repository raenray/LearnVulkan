#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>

namespace Engine { namespace Renderer {

// Thin RAII wrapper around the Vulkan Memory Allocator (VMA) library.
// Owns a VmaAllocator; provides allocation helpers for buffers and images.

class GpuAllocator {
public:
    GpuAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
                 uint32_t apiVersionMajor = 1, uint32_t apiVersionMinor = 0);
    ~GpuAllocator();
    GpuAllocator(const GpuAllocator&) = delete;
    GpuAllocator& operator=(const GpuAllocator&) = delete;
    GpuAllocator(GpuAllocator&&) noexcept;
    GpuAllocator& operator=(GpuAllocator&&) noexcept;

    VmaAllocator getAllocator() const { return allocator_; }
    void* handle() const { return reinterpret_cast<void*>(allocator_); }
    operator VmaAllocator() const { return allocator_; }

    VkResult createImage(const VkImageCreateInfo* pCI, VkImage* pImage, void** pAlloc) const;
    void destroyImage(VkImage image, void* alloc) const;

private:
    VmaAllocator allocator_ = VK_NULL_HANDLE;
};

} }
