#include "Engine/Renderer/RenderGraph/render_graph.hpp"
#include <stdexcept>
#include <cstring>

namespace Engine
{
namespace Renderer
{

RenderGraph::RenderGraph(VkDevice device, VkPhysicalDevice physicalDevice)
    : device_(device)
    , physicalDevice_(physicalDevice)
{
}

RenderGraph::~RenderGraph()
{
    for (auto& img : images_)
    {
        if (img.owned)
        {
            if (img.view)
                vkDestroyImageView(device_, img.view, nullptr);
            if (img.image)
                vkDestroyImage(device_, img.image, nullptr);
            if (img.memory)
                vkFreeMemory(device_, img.memory, nullptr);
        }
    }
    for (auto& buf : buffers_)
    {
        if (buf.owned)
        {
            if (buf.buffer)
                vkDestroyBuffer(device_, buf.buffer, nullptr);
            if (buf.memory)
                vkFreeMemory(device_, buf.memory, nullptr);
        }
    }
}

RGHandle RenderGraph::createImage(const std::string& name, const RGImageDesc& desc)
{
    ImageResource res;
    res.name = name;
    res.desc = desc;
    res.owned = true;

    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = desc.format;
    ci.extent = {desc.extent.width, desc.extent.height, 1};
    ci.mipLevels = desc.mipLevels;
    ci.arrayLayers = desc.arrayLayers;
    ci.samples = desc.samples;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = desc.usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device_, &ci, nullptr, &res.image) != VK_SUCCESS)
        throw std::runtime_error("RenderGraph: failed to create image '" + name + "'");

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device_, res.image, &memReq);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProps);
    uint32_t memTypeIndex = 0;
    for (; memTypeIndex < memProps.memoryTypeCount; ++memTypeIndex)
    {
        if ((memReq.memoryTypeBits & (1 << memTypeIndex)) &&
            (memProps.memoryTypes[memTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            break;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memTypeIndex;

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &res.memory) != VK_SUCCESS)
        throw std::runtime_error("RenderGraph: failed to allocate image memory '" + name + "'");
    vkBindImageMemory(device_, res.image, res.memory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = res.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = desc.format;
    viewInfo.subresourceRange.aspectMask = desc.aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = desc.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = desc.arrayLayers;

    if (vkCreateImageView(device_, &viewInfo, nullptr, &res.view) != VK_SUCCESS)
        throw std::runtime_error("RenderGraph: failed to create image view '" + name + "'");

    RGHandle h;
    h.index = static_cast<uint32_t>(images_.size());
    images_.push_back(std::move(res));
    return h;
}

RGHandle RenderGraph::createBuffer(const std::string& name, const RGBufferDesc& desc)
{
    BufferResource res;
    res.name = name;
    res.desc = desc;
    res.owned = true;

    VkBufferCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = desc.size;
    ci.usage = desc.usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &ci, nullptr, &res.buffer) != VK_SUCCESS)
        throw std::runtime_error("RenderGraph: failed to create buffer '" + name + "'");

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device_, res.buffer, &memReq);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProps);
    uint32_t memTypeIndex = 0;
    for (; memTypeIndex < memProps.memoryTypeCount; ++memTypeIndex)
    {
        if ((memReq.memoryTypeBits & (1 << memTypeIndex)) &&
            (memProps.memoryTypes[memTypeIndex].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
            break;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memTypeIndex;

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &res.memory) != VK_SUCCESS)
        throw std::runtime_error("RenderGraph: failed to allocate buffer memory '" + name + "'");
    vkBindBufferMemory(device_, res.buffer, res.memory, 0);

    RGHandle h;
    h.index = static_cast<uint32_t>(buffers_.size());
    buffers_.push_back(std::move(res));
    return h;
}

RGHandle RenderGraph::importImage(const std::string& name, VkImage image, VkImageView view, const RGImageDesc& desc, VkImageLayout currentLayout)
{
    ImageResource res;
    res.name = name;
    res.image = image;
    res.view = view;
    res.desc = desc;
    res.currentLayout = currentLayout;
    res.owned = false;

    RGHandle h;
    h.index = static_cast<uint32_t>(images_.size());
    images_.push_back(std::move(res));
    return h;
}

VkImage RenderGraph::getImage(RGHandle h) const
{
    return (h.valid() && h.index < images_.size()) ? images_[h.index].image : VK_NULL_HANDLE;
}

VkImageView RenderGraph::getImageView(RGHandle h) const
{
    return (h.valid() && h.index < images_.size()) ? images_[h.index].view : VK_NULL_HANDLE;
}

VkBuffer RenderGraph::getBuffer(RGHandle h) const
{
    return (h.valid() && h.index < buffers_.size()) ? buffers_[h.index].buffer : VK_NULL_HANDLE;
}

void RenderGraph::beginFrame()
{
    // In a real engine, this would reset transient resources and free old ones.
    // For now, resources persist across frames.
}

void RenderGraph::compile()
{
    // In a full engine, this would compile the graph: insert barriers, sort passes, etc.
}

void RenderGraph::imageBarrier(VkCommandBuffer cmd,
                               RGHandle h,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               VkPipelineStageFlags srcStage,
                               VkPipelineStageFlags dstStage,
                               VkAccessFlags srcAccess,
                               VkAccessFlags dstAccess)
{
    if (!h.valid() || h.index >= images_.size())
        return;
    auto& img = images_[h.index];

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = img.image;
    barrier.subresourceRange.aspectMask = img.desc.aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = img.desc.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = img.desc.arrayLayers;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    img.currentLayout = newLayout;
}

} // namespace Renderer
} // namespace Engine
