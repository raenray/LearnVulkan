#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "Engine/Renderer/Vulkan/vulkan_queue.hpp"
namespace Engine { namespace Renderer {
struct VulkanDeviceConfig { uint32_t apiMajor=1, apiMinor=0; };
class VulkanDevice {
public:
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanDeviceConfig& cfg={});
    ~VulkanDevice();
    VulkanDevice(const VulkanDevice&)=delete; VulkanDevice& operator=(const VulkanDevice&)=delete;
    VulkanDevice(VulkanDevice&&)noexcept; VulkanDevice& operator=(VulkanDevice&&)noexcept;
    VkPhysicalDevice physicalDevice() const { return pd_; }
    VkDevice device() const { return d_; }
    VulkanQueue graphicsQueue() const { return {gq_,gf_}; }
    VulkanQueue presentQueue() const { return {pq_,pf_}; }
private:
    struct Impl; std::unique_ptr<Impl> impl_;
    VkPhysicalDevice pd_=VK_NULL_HANDLE; VkDevice d_=VK_NULL_HANDLE; uint32_t gf_=0,pf_=0;
    VkQueue gq_=VK_NULL_HANDLE, pq_=VK_NULL_HANDLE;
};
} }
