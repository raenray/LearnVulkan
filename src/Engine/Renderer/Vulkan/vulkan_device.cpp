#include "Engine/Renderer/Vulkan/vulkan_device.hpp"
#include <VkBootstrap.h>
#include <stdexcept>

namespace Engine
{
namespace Renderer
{
struct VulkanDevice::Impl
{
    vkb::Device vkbDevice;
};

VulkanDevice::VulkanDevice(VkInstance inst, VkSurfaceKHR surf, const VulkanDeviceConfig& cfg)
    : impl_(std::make_unique<Impl>())
{
    vkb::Instance vi;
    vi.instance = inst;
    vi.fp_vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    auto p = vkb::PhysicalDeviceSelector(vi).set_surface(surf).set_minimum_version(cfg.apiMajor, cfg.apiMinor).select();
    if (!p)
        throw std::runtime_error("VulkanDevice: " + p.error().message());
    pd_ = p.value().physical_device;
    auto d = vkb::DeviceBuilder(p.value()).build();
    if (!d)
        throw std::runtime_error("VulkanDevice: " + d.error().message());
    impl_->vkbDevice = d.value();
    d_ = impl_->vkbDevice.device;
    auto gq = impl_->vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!gq)
        throw std::runtime_error("no gfx queue");
    gq_ = gq.value();
    auto gqi = impl_->vkbDevice.get_queue_index(vkb::QueueType::graphics);
    if (gqi)
        gf_ = gqi.value();
    auto pq = impl_->vkbDevice.get_queue(vkb::QueueType::present);
    if (!pq)
        throw std::runtime_error("no present queue");
    pq_ = pq.value();
    auto pqi = impl_->vkbDevice.get_queue_index(vkb::QueueType::present);
    if (pqi)
        pf_ = pqi.value();
}

VulkanDevice::~VulkanDevice() = default;
VulkanDevice::VulkanDevice(VulkanDevice&&) noexcept = default;
VulkanDevice& VulkanDevice::operator=(VulkanDevice&&) noexcept = default;
} // namespace Renderer
} // namespace Engine
