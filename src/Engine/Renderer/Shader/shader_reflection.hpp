#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Engine
{
namespace Renderer
{
struct ShaderDescriptorBinding
{
    uint32_t binding = 0;
    uint32_t set = 0;
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    uint32_t count = 1;
    std::string name;
};

struct ShaderPushConstantRange
{
    VkShaderStageFlags stage = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
};

struct ShaderReflection
{
    VkShaderStageFlags stage = 0;
    std::string entryPoint = "main";
    std::vector<ShaderDescriptorBinding> bindings;
    std::vector<ShaderPushConstantRange> pushConstants;
};
} // namespace Renderer
} // namespace Engine
