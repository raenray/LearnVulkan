#include "Engine/Renderer/Vulkan/vulkan_swapchain.hpp"
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <limits>

namespace Engine
{
namespace Renderer
{

// --- Query support details ---
VulkanSwapchainSupportDetails VulkanSwapchain::querySupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VulkanSwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
    if (count)
    {
        details.formats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, details.formats.data());
    }
    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
    if (count)
    {
        details.presentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available)
{
    for (const auto& f : available)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return available[0];
}

VkPresentModeKHR VulkanSwapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& available)
{
    for (const auto& m : available)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR)
            return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& caps, uint32_t w, uint32_t h)
{
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;
    return {std::clamp(w, caps.minImageExtent.width, caps.maxImageExtent.width),
            std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height)};
}

// --- Constructor / Destructor ---

VulkanSwapchain::VulkanSwapchain(VkPhysicalDevice pd, VkDevice d, VkSurfaceKHR surf, uint32_t w, uint32_t h, VkSwapchainKHR old)
    : physicalDevice_(pd)
    , device_(d)
{
    createSwapchain(pd, d, surf, w, h, old);
    createImageViews(d);
    createRenderPass(d);
    createFramebuffers(d);
}

VulkanSwapchain::~VulkanSwapchain()
{
    if (device_)
    {
        for (auto fb : framebuffers_)
            vkDestroyFramebuffer(device_, fb, nullptr);
        if (renderPass_)
            vkDestroyRenderPass(device_, renderPass_, nullptr);
        for (auto iv : imageViews_)
            vkDestroyImageView(device_, iv, nullptr);
        if (swapchain_)
            vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }
}

// --- Private helpers ---

void VulkanSwapchain::createSwapchain(VkPhysicalDevice pd, VkDevice d, VkSurfaceKHR surf, uint32_t w, uint32_t h, VkSwapchainKHR old)
{
    auto support = querySupport(pd, surf);
    auto fmt = chooseSurfaceFormat(support.formats);
    auto mode = choosePresentMode(support.presentModes);
    auto ext = chooseExtent(support.capabilities, w, h);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
        imageCount = support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = surf;
    ci.minImageCount = imageCount;
    ci.imageFormat = fmt.format;
    ci.imageColorSpace = fmt.colorSpace;
    ci.imageExtent = ext;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.preTransform = support.capabilities.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = mode;
    ci.clipped = VK_TRUE;
    ci.oldSwapchain = old;

    // We assume graphics == present queue family for simplicity (common case)
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.queueFamilyIndexCount = 0;
    ci.pQueueFamilyIndices = nullptr;

    if (vkCreateSwapchainKHR(d, &ci, nullptr, &swapchain_) != VK_SUCCESS)
        throw std::runtime_error("VulkanSwapchain: failed to create swapchain");

    imageFormat_ = fmt.format;
    extent_ = ext;

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(d, swapchain_, &count, nullptr);
    images_.resize(count);
    vkGetSwapchainImagesKHR(d, swapchain_, &count, images_.data());
}

void VulkanSwapchain::createImageViews(VkDevice d)
{
    imageViews_.resize(images_.size());
    for (size_t i = 0; i < images_.size(); ++i)
    {
        VkImageViewCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.image = images_[i];
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format = imageFormat_;
        ci.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        if (vkCreateImageView(d, &ci, nullptr, &imageViews_[i]) != VK_SUCCESS)
            throw std::runtime_error("VulkanSwapchain: failed to create image view");
    }
}

void VulkanSwapchain::createRenderPass(VkDevice d)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = imageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rp{};
    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp.attachmentCount = 1;
    rp.pAttachments = &colorAttachment;
    rp.subpassCount = 1;
    rp.pSubpasses = &subpass;
    rp.dependencyCount = 1;
    rp.pDependencies = &dep;

    if (vkCreateRenderPass(d, &rp, nullptr, &renderPass_) != VK_SUCCESS)
        throw std::runtime_error("VulkanSwapchain: failed to create render pass");
}

void VulkanSwapchain::createFramebuffers(VkDevice d)
{
    framebuffers_.resize(imageViews_.size());
    for (size_t i = 0; i < imageViews_.size(); ++i)
    {
        VkImageView attachments[] = {imageViews_[i]};
        VkFramebufferCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.renderPass = renderPass_;
        ci.attachmentCount = 1;
        ci.pAttachments = attachments;
        ci.width = extent_.width;
        ci.height = extent_.height;
        ci.layers = 1;
        if (vkCreateFramebuffer(d, &ci, nullptr, &framebuffers_[i]) != VK_SUCCESS)
            throw std::runtime_error("VulkanSwapchain: failed to create framebuffer");
    }
}

} // namespace Renderer
} // namespace Engine
