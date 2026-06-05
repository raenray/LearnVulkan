#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

namespace Engine
{
namespace Renderer
{

// RAII VkSampler wrapper
struct GpuSamplerDesc
{
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkFilter minFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float mipLodBias = 0.0f;
    VkBool32 anisotropyEnable = VK_FALSE;
    float maxAnisotropy = 1.0f;
    VkBool32 compareEnable = VK_FALSE;
    VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
    float minLod = 0.0f;
    float maxLod = VK_LOD_CLAMP_NONE;
    VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    VkBool32 unnormalizedCoordinates = VK_FALSE;
};

class GpuSampler
{
public:
    GpuSampler() = default;
    GpuSampler(VkDevice device, const GpuSamplerDesc& desc);
    ~GpuSampler();
    GpuSampler(const GpuSampler&) = delete;
    GpuSampler& operator=(const GpuSampler&) = delete;
    GpuSampler(GpuSampler&& other) noexcept;
    GpuSampler& operator=(GpuSampler&& other) noexcept;

    VkSampler get() const { return sampler_; }

    operator VkSampler() const { return sampler_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

} // namespace Renderer
} // namespace Engine
