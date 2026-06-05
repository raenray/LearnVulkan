#include "Engine/Renderer/Resource/gpu_sampler.hpp"
#include <stdexcept>

namespace Engine
{
namespace Renderer
{

GpuSampler::GpuSampler(VkDevice device, const GpuSamplerDesc& desc)
    : device_(device)
{
    VkSamplerCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter = desc.magFilter;
    ci.minFilter = desc.minFilter;
    ci.mipmapMode = desc.mipmapMode;
    ci.addressModeU = desc.addressModeU;
    ci.addressModeV = desc.addressModeV;
    ci.addressModeW = desc.addressModeW;
    ci.mipLodBias = desc.mipLodBias;
    ci.anisotropyEnable = desc.anisotropyEnable;
    ci.maxAnisotropy = desc.maxAnisotropy;
    ci.compareEnable = desc.compareEnable;
    ci.compareOp = desc.compareOp;
    ci.minLod = desc.minLod;
    ci.maxLod = desc.maxLod;
    ci.borderColor = desc.borderColor;
    ci.unnormalizedCoordinates = desc.unnormalizedCoordinates;

    if (vkCreateSampler(device, &ci, nullptr, &sampler_) != VK_SUCCESS)
        throw std::runtime_error("GpuSampler: failed to create sampler");
}

GpuSampler::~GpuSampler()
{
    if (sampler_ && device_)
        vkDestroySampler(device_, sampler_, nullptr);
}

GpuSampler::GpuSampler(GpuSampler&& other) noexcept
    : device_(other.device_)
    , sampler_(other.sampler_)
{
    other.device_ = VK_NULL_HANDLE;
    other.sampler_ = VK_NULL_HANDLE;
}

GpuSampler& GpuSampler::operator=(GpuSampler&& other) noexcept
{
    if (this != &other)
    {
        if (sampler_ && device_)
            vkDestroySampler(device_, sampler_, nullptr);
        device_ = other.device_;
        sampler_ = other.sampler_;
        other.device_ = VK_NULL_HANDLE;
        other.sampler_ = VK_NULL_HANDLE;
    }
    return *this;
}

} // namespace Renderer
} // namespace Engine
