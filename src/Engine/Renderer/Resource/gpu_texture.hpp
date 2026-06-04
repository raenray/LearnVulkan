#pragma once
#include <vulkan/vulkan.h>
namespace Engine { namespace Renderer {
struct GPUTextureCreateInfo { VkImageType imageType=VK_IMAGE_TYPE_2D; VkFormat format=VK_FORMAT_UNDEFINED; VkExtent3D extent{0,0,0}; uint32_t mipLevels=1,arrayLayers=1; VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT; VkImageTiling tiling=VK_IMAGE_TILING_OPTIMAL; VkImageUsageFlags usage=0; VkImageLayout initialLayout=VK_IMAGE_LAYOUT_UNDEFINED; const char* name=nullptr; };
struct GPUTextureViewCreateInfo { VkImageViewType viewType=VK_IMAGE_VIEW_TYPE_2D; VkComponentMapping components={VK_COMPONENT_SWIZZLE_IDENTITY}; VkImageSubresourceRange sr{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}; };
class GPUTexture {
public:
    GPUTexture(void* vmaAlloc, const GPUTextureCreateInfo& ci, const GPUTextureViewCreateInfo* vci=nullptr);
    ~GPUTexture();
    GPUTexture(const GPUTexture&)=delete; GPUTexture& operator=(const GPUTexture&)=delete;
    GPUTexture(GPUTexture&&)noexcept; GPUTexture& operator=(GPUTexture&&)noexcept;
    VkImage image()const{return img_;} VkImageView imageView()const{return view_;} VkFormat fmt()const{return fmt_;} VkExtent3D ext()const{return ext_;} uint32_t mips()const{return mips_;}
private:
    VkDevice dev_=VK_NULL_HANDLE; void* allocator_=nullptr; VkImage img_=VK_NULL_HANDLE; void* allocation_=nullptr; VkImageView view_=VK_NULL_HANDLE; VkFormat fmt_=VK_FORMAT_UNDEFINED; VkExtent3D ext_{}; uint32_t mips_=1;
};
} }
