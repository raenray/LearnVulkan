#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <memory>

namespace Engine
{
namespace Renderer
{

struct VulkanSwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain
{
public:
    VulkanSwapchain(VkPhysicalDevice physicalDevice,
                    VkDevice device,
                    VkSurfaceKHR surface,
                    uint32_t width,
                    uint32_t height,
                    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    ~VulkanSwapchain();
    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    VkSwapchainKHR swapchain() const { return swapchain_; }

    VkFormat imageFormat() const { return imageFormat_; }

    VkExtent2D extent() const { return extent_; }

    VkRenderPass renderPass() const { return renderPass_; }

    uint32_t imageCount() const { return static_cast<uint32_t>(imageViews_.size()); }

    const std::vector<VkImageView>& imageViews() const { return imageViews_; }

    const std::vector<VkFramebuffer>& framebuffers() const { return framebuffers_; }

    static VulkanSwapchainSupportDetails querySupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available);
    static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& available);
    static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

private:
    void createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain);
    void createImageViews(VkDevice device);
    void createRenderPass(VkDevice device);
    void createFramebuffers(VkDevice device);

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat imageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    std::vector<VkFramebuffer> framebuffers_;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
};

} // namespace Renderer
} // namespace Engine
