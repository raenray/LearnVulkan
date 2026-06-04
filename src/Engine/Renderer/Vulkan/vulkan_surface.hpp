#pragma once
#include <vulkan/vulkan.h>

namespace Engine { namespace Renderer {

// Simple RAII wrapper for VkSurfaceKHR
class VulkanSurface {
public:
    VulkanSurface(VkInstance instance, VkSurfaceKHR surface) : instance_(instance), surface_(surface) {}
    ~VulkanSurface() { if (surface_ && instance_) vkDestroySurfaceKHR(instance_, surface_, nullptr); }
    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;
    VulkanSurface(VulkanSurface&& other) noexcept : instance_(other.instance_), surface_(other.surface_) { other.surface_ = VK_NULL_HANDLE; other.instance_ = VK_NULL_HANDLE; }
    VulkanSurface& operator=(VulkanSurface&& other) noexcept {
        if (this != &other) {
            if (surface_ && instance_) vkDestroySurfaceKHR(instance_, surface_, nullptr);
            instance_ = other.instance_; surface_ = other.surface_;
            other.instance_ = VK_NULL_HANDLE; other.surface_ = VK_NULL_HANDLE;
        }
        return *this;
    }
    VkSurfaceKHR get() const { return surface_; }
    operator VkSurfaceKHR() const { return surface_; }
private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
};

} }
