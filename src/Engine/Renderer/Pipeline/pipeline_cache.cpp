#include "Engine/Renderer/Pipeline/pipeline_cache.hpp"
#include <stdexcept>

namespace Engine
{
namespace Renderer
{

PipelineCache::PipelineCache(VkDevice device)
    : device_(device)
{
    VkPipelineCacheCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    if (vkCreatePipelineCache(device, &ci, nullptr, &cache_) != VK_SUCCESS)
        throw std::runtime_error("PipelineCache: failed to create pipeline cache");
}

PipelineCache::~PipelineCache()
{
    clear();
    if (cache_)
        vkDestroyPipelineCache(device_, cache_, nullptr);
}

void PipelineCache::put(const std::string& name, VkPipeline pipeline)
{
    auto it = pipelines_.find(name);
    if (it != pipelines_.end() && it->second)
        vkDestroyPipeline(device_, it->second, nullptr);
    pipelines_[name] = pipeline;
}

VkPipeline PipelineCache::get(const std::string& name) const
{
    auto it = pipelines_.find(name);
    return (it != pipelines_.end()) ? it->second : VK_NULL_HANDLE;
}

void PipelineCache::remove(const std::string& name)
{
    auto it = pipelines_.find(name);
    if (it != pipelines_.end())
    {
        if (it->second)
            vkDestroyPipeline(device_, it->second, nullptr);
        pipelines_.erase(it);
    }
}

void PipelineCache::clear()
{
    for (auto& [_, pipeline] : pipelines_)
        if (pipeline)
            vkDestroyPipeline(device_, pipeline, nullptr);
    pipelines_.clear();
}

void PipelineCache::merge(const std::vector<VkPipelineCache>& caches)
{
    if (!caches.empty())
        vkMergePipelineCaches(device_, cache_, static_cast<uint32_t>(caches.size()), caches.data());
}

std::vector<uint8_t> PipelineCache::getData() const
{
    size_t size = 0;
    vkGetPipelineCacheData(device_, cache_, &size, nullptr);
    std::vector<uint8_t> data(size);
    if (size > 0)
        vkGetPipelineCacheData(device_, cache_, &size, data.data());
    return data;
}

} // namespace Renderer
} // namespace Engine
