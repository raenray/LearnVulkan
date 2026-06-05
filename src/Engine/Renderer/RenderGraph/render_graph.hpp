#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>

namespace Engine
{
namespace Renderer
{

// Lightweight frame-graph style resource manager for transient render targets
// Tracks images, buffers, and barriers for a single frame's passes.
// No complex graph scheduling — just a linear list of passes with resource handles.

struct RGHandle
{
    uint32_t index = ~0u;

    bool valid() const { return index != ~0u; }
};

struct RGImageDesc
{
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    VkImageUsageFlags usage = 0;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
};

struct RGBufferDesc
{
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
};

class RenderGraph
{
public:
    RenderGraph(VkDevice device, VkPhysicalDevice physicalDevice);
    ~RenderGraph();
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    // Resource creation
    RGHandle createImage(const std::string& name, const RGImageDesc& desc);
    RGHandle createBuffer(const std::string& name, const RGBufferDesc& desc);

    // Import external resources (e.g. swapchain images)
    RGHandle importImage(const std::string& name, VkImage image, VkImageView view, const RGImageDesc& desc, VkImageLayout currentLayout);

    // Access underlying Vulkan resources
    VkImage getImage(RGHandle h) const;
    VkImageView getImageView(RGHandle h) const;
    VkBuffer getBuffer(RGHandle h) const;

    // Begin a new frame: reset transient resources
    void beginFrame();

    // Execute all recorded passes (for now, a no-op — in a full engine this
    // would compile a command list and insert barriers)
    void compile();

    // Barrier helper: insert an image memory barrier into the current command buffer
    void imageBarrier(VkCommandBuffer cmd,
                      RGHandle h,
                      VkImageLayout oldLayout,
                      VkImageLayout newLayout,
                      VkPipelineStageFlags srcStage,
                      VkPipelineStageFlags dstStage,
                      VkAccessFlags srcAccess,
                      VkAccessFlags dstAccess);

private:
    struct ImageResource
    {
        std::string name;
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        RGImageDesc desc;
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        bool owned = true; // false if imported
    };

    struct BufferResource
    {
        std::string name;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        RGBufferDesc desc;
        bool owned = true;
    };

    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    std::vector<ImageResource> images_;
    std::vector<BufferResource> buffers_;
};

} // namespace Renderer
} // namespace Engine
