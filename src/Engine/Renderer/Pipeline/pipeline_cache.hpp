#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace Engine
{
namespace Renderer
{

// Caches VkPipeline objects keyed by a hash of their create info.
// Integrates with ShaderManager to avoid rebuilding pipelines every frame.

class PipelineCache
{
public:
    PipelineCache(VkDevice device);
    ~PipelineCache();
    PipelineCache(const PipelineCache&) = delete;
    PipelineCache& operator=(const PipelineCache&) = delete;

    // Store a pipeline under a name
    void put(const std::string& name, VkPipeline pipeline);

    // Retrieve a cached pipeline, or VK_NULL_HANDLE if not found
    VkPipeline get(const std::string& name) const;

    // Remove a pipeline and destroy it
    void remove(const std::string& name);

    // Destroy all cached pipelines
    void clear();

    // Direct access to the VkPipelineCache object for serialization
    VkPipelineCache vkCache() const { return cache_; }

    // Merge pipeline caches from disk
    void merge(const std::vector<VkPipelineCache>& caches);

    // Get serialized cache data for persistence
    std::vector<uint8_t> getData() const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipelineCache cache_ = VK_NULL_HANDLE;
    std::unordered_map<std::string, VkPipeline> pipelines_;
};

} // namespace Renderer
} // namespace Engine
