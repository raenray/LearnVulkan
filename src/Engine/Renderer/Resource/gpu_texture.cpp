#include "Engine/Renderer/Resource/gpu_texture.hpp"
#include <vk_mem_alloc.h>
#include <stdexcept>
namespace Engine { namespace Renderer {
static inline VmaAllocator toVma(void*h){return reinterpret_cast<VmaAllocator>(h);}
static inline VmaAllocation toVmaAlloc(void*h){return reinterpret_cast<VmaAllocation>(h);}
GPUTexture::GPUTexture(void* vma, const GPUTextureCreateInfo& ci, const GPUTextureViewCreateInfo* vci) : allocator_(vma), fmt_(ci.format), ext_(ci.extent), mips_(ci.mipLevels) {
    auto* a=toVma(vma); VmaAllocatorInfo ai; vmaGetAllocatorInfo(a,&ai); dev_=ai.device;
    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO}; ici.imageType=ci.imageType; ici.format=ci.format; ici.extent=ci.extent; ici.mipLevels=ci.mipLevels; ici.arrayLayers=ci.arrayLayers; ici.samples=ci.samples; ici.tiling=ci.tiling; ici.usage=ci.usage; ici.initialLayout=ci.initialLayout;
    VmaAllocationCreateInfo aci{}; aci.usage=VMA_MEMORY_USAGE_AUTO;
    VkImage im;VmaAllocation al;
    if(vmaCreateImage(a,&ici,&aci,&im,&al,nullptr)!=VK_SUCCESS){std::string msg="GPUTexture: vmaCreateImage failed";if(ci.name){msg+=" — ";msg+=ci.name;}throw std::runtime_error(msg);}
    img_=im;allocation_=reinterpret_cast<void*>(al);if(ci.name)vmaSetAllocationName(a,al,ci.name);
    if(vci){VkImageViewCreateInfo vci2{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vci2.image=img_;vci2.viewType=vci->viewType;vci2.format=ci.format;vci2.components=vci->components;vci2.subresourceRange=vci->sr;vkCreateImageView(dev_,&vci2,nullptr,&view_);}
}
GPUTexture::~GPUTexture(){if(view_)vkDestroyImageView(dev_,view_,nullptr);if(img_&&allocator_)vmaDestroyImage(toVma(allocator_),img_,toVmaAlloc(allocation_));}
GPUTexture::GPUTexture(GPUTexture&&o)noexcept:dev_(o.dev_),allocator_(o.allocator_),img_(o.img_),allocation_(o.allocation_),view_(o.view_),fmt_(o.fmt_),ext_(o.ext_),mips_(o.mips_){o.dev_=VK_NULL_HANDLE;o.allocator_=nullptr;o.img_=VK_NULL_HANDLE;o.allocation_=nullptr;o.view_=VK_NULL_HANDLE;}
GPUTexture& GPUTexture::operator=(GPUTexture&&o)noexcept{if(this!=&o){if(view_)vkDestroyImageView(dev_,view_,nullptr);if(img_&&allocator_)vmaDestroyImage(toVma(allocator_),img_,toVmaAlloc(allocation_));dev_=o.dev_;allocator_=o.allocator_;img_=o.img_;allocation_=o.allocation_;view_=o.view_;fmt_=o.fmt_;ext_=o.ext_;mips_=o.mips_;o.dev_=VK_NULL_HANDLE;o.allocator_=nullptr;o.img_=VK_NULL_HANDLE;o.allocation_=nullptr;o.view_=VK_NULL_HANDLE;}return*this;}
} }
