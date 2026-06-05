#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <fstream>

namespace Engine
{
namespace Renderer
{

// Simple compute pipeline abstraction.
// Holds a VkPipeline + VkPipelineLayout + VkDescriptorSetLayout for a compute shader.

struct ComputePipelineDesc
{
    std::string shaderPath;
    std::vector<VkDescriptorSetLayout> extraSetLayouts;
    std::vector<VkPushConstantRange> pushConstants;
};

class ComputePipeline
{
public:
    ComputePipeline(VkDevice device, const ComputePipelineDesc& desc, VkPipelineCache pipelineCache = VK_NULL_HANDLE);
    ~ComputePipeline();
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    VkPipeline pipeline() const { return pipeline_; }

    VkPipelineLayout layout() const { return layout_; }

    VkDescriptorSetLayout descriptorSetLayout() const { return descriptorSetLayout_; }

    void dispatch(VkCommandBuffer cmd, uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;

    static std::vector<char> readFile(const std::string& path);
    VkShaderModule createShaderModule(const std::vector<char>& code);
};

// --- Inline implementations ---

inline ComputePipeline::ComputePipeline(VkDevice device, const ComputePipelineDesc& desc, VkPipelineCache pipelineCache)
    : device_(device)
{
    // Load compute shader
    auto code = readFile(desc.shaderPath);
    VkShaderModule shaderModule = createShaderModule(code);

    // Create descriptor set layout (empty for now — real engine would use reflection)
    VkDescriptorSetLayoutCreateInfo dslInfo{};
    dslInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // Bindings would come from shader reflection
    if (vkCreateDescriptorSetLayout(device, &dslInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        throw std::runtime_error("ComputePipeline: failed to create descriptor set layout");
    }

    // Gather all descriptor set layouts
    std::vector<VkDescriptorSetLayout> setLayouts = {descriptorSetLayout_};
    for (auto& extra : desc.extraSetLayouts)
        setLayouts.push_back(extra);

    // Create pipeline layout
    VkPipelineLayoutCreateInfo plInfo{};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    plInfo.pSetLayouts = setLayouts.data();
    plInfo.pushConstantRangeCount = static_cast<uint32_t>(desc.pushConstants.size());
    plInfo.pPushConstantRanges = desc.pushConstants.data();

    if (vkCreatePipelineLayout(device, &plInfo, nullptr, &layout_) != VK_SUCCESS)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        throw std::runtime_error("ComputePipeline: failed to create pipeline layout");
    }

    // Create compute pipeline
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeInfo.stage = stageInfo;
    pipeInfo.layout = layout_;

    if (vkCreateComputePipelines(device, pipelineCache, 1, &pipeInfo, nullptr, &pipeline_) != VK_SUCCESS)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        throw std::runtime_error("ComputePipeline: failed to create compute pipeline");
    }

    vkDestroyShaderModule(device, shaderModule, nullptr);
}

inline ComputePipeline::~ComputePipeline()
{
    if (pipeline_)
        vkDestroyPipeline(device_, pipeline_, nullptr);
    if (layout_)
        vkDestroyPipelineLayout(device_, layout_, nullptr);
    if (descriptorSetLayout_)
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
}

inline void ComputePipeline::dispatch(VkCommandBuffer cmd, uint32_t gx, uint32_t gy, uint32_t gz) const
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdDispatch(cmd, gx, gy, gz);
}

inline std::vector<char> ComputePipeline::readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("ComputePipeline: failed to open '" + path + "'");
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(size));
    return buffer;
}

inline VkShaderModule ComputePipeline::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    if (vkCreateShaderModule(device_, &ci, nullptr, &module) != VK_SUCCESS)
        throw std::runtime_error("ComputePipeline: failed to create shader module");
    return module;
}

} // namespace Renderer
} // namespace Engine
