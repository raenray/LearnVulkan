#pragma once
#include <vulkan/vulkan.h>
#include <memory>
struct GLFWwindow;

namespace Engine
{
namespace Renderer
{
class VulkanDevice;
class VulkanSwapchain;
struct VulkanQueue;
typedef VulkanQueue VkQueueBundle;
} // namespace Renderer
} // namespace Engine

#include "Engine/Renderer/Vulkan/vulkan_device.hpp"
#include "Engine/Renderer/Vulkan/vulkan_swapchain.hpp"
#include "Engine/Renderer/Vulkan/vulkan_queue.hpp"
#include <functional>

namespace Engine
{
namespace Renderer
{
struct VulkanContextConfig
{
    const char* appName = "LearnVulkan";
    bool enableValidation = true;
    uint32_t apiMajor = 1, apiMinor = 0;
};

class VulkanContext
{
public:
    VulkanContext(GLFWwindow* w, const VulkanContextConfig& c = {});
    ~VulkanContext();
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    VkInstance instance() const { return inst_; }

    VkSurfaceKHR surface() const { return surf_; }

    VkPhysicalDevice physicalDevice() const { return dev_->physicalDevice(); }

    VkDevice device() const { return dev_->device(); }

    VulkanSwapchain& swapchain() { return *sw_; }

    const VulkanSwapchain& swapchain() const { return *sw_; }

    void recreateSwapchain(uint32_t w, uint32_t h);

    VkQueue graphicsQueue() const { return dev_->graphicsQueue().handle; }

    uint32_t graphicsQueueFamily() const { return dev_->graphicsQueue().family; }

    VkQueue presentQueue() const { return dev_->presentQueue().handle; }

    uint32_t presentQueueFamily() const { return dev_->presentQueue().family; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::unique_ptr<VulkanDevice> dev_;
    std::unique_ptr<VulkanSwapchain> sw_;
    VkInstance inst_ = VK_NULL_HANDLE;
    VkSurfaceKHR surf_ = VK_NULL_HANDLE;
};
} // namespace Renderer
} // namespace Engine
