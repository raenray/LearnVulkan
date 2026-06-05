#include "Engine/Renderer/Vulkan/vulkan_context.hpp"
#include <VkBootstrap.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>
#include <iostream>

namespace Engine
{
namespace Renderer
{
struct VulkanContext::Impl
{
    vkb::Instance vkbInst;
};

VulkanContext::VulkanContext(GLFWwindow* w, const VulkanContextConfig& cfg)
    : impl_(std::make_unique<Impl>())
{
    auto ib = vkb::InstanceBuilder().set_app_name(cfg.appName).require_api_version(cfg.apiMajor, cfg.apiMinor);
    if (cfg.enableValidation)
    {
        ib.request_validation_layers().use_default_debug_messenger();
    }
    auto ir = ib.build();
    if (!ir)
    {
        throw std::runtime_error("VulkanContext: instance creation failed — " + ir.error().message());
    }
    impl_->vkbInst = ir.value();
    inst_ = impl_->vkbInst.instance;
    if (glfwCreateWindowSurface(inst_, w, nullptr, &surf_) != VK_SUCCESS)
        throw std::runtime_error("VulkanContext: surface failed");
    dev_ = std::make_unique<VulkanDevice>(inst_, surf_, VulkanDeviceConfig{cfg.apiMajor, cfg.apiMinor});
    int ww, hh;
    glfwGetFramebufferSize(w, &ww, &hh);
    sw_ = std::make_unique<VulkanSwapchain>(dev_->physicalDevice(), dev_->device(), surf_, (uint32_t) ww, (uint32_t) hh);
}

VulkanContext::~VulkanContext()
{
    if (dev_ && dev_->device())
        vkDeviceWaitIdle(dev_->device());
    sw_.reset();
    dev_.reset();
    if (surf_)
    {
        vkDestroySurfaceKHR(inst_, surf_, nullptr);
        surf_ = VK_NULL_HANDLE;
    }
}

void VulkanContext::recreateSwapchain(uint32_t w, uint32_t h)
{
    if (!dev_ || !dev_->device())
        return;
    vkDeviceWaitIdle(dev_->device());
    VkSwapchainKHR old = sw_ ? sw_->swapchain() : VK_NULL_HANDLE;
    sw_ = std::make_unique<VulkanSwapchain>(dev_->physicalDevice(), dev_->device(), surf_, w, h, old);
}

} // namespace Renderer
} // namespace Engine
