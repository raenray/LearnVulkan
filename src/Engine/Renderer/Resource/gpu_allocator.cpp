#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
#define VMA_IMPLEMENTATION
#include "Engine/Renderer/Resource/gpu_allocator.hpp"
#include <stdexcept>
namespace Engine { namespace Renderer {
GpuAllocator::GpuAllocator(VkInstance inst, VkPhysicalDevice pd, VkDevice d, uint32_t maj, uint32_t min) {
    VmaVulkanFunctions f{}; f.vkGetInstanceProcAddr=vkGetInstanceProcAddr; f.vkGetDeviceProcAddr=vkGetDeviceProcAddr;
    VmaAllocatorCreateInfo ci{}; ci.vulkanApiVersion=VK_MAKE_API_VERSION(0,maj,min,0); ci.physicalDevice=pd; ci.device=d; ci.instance=inst; ci.pVulkanFunctions=&f;
    if(vmaCreateAllocator(&ci,&allocator_)!=VK_SUCCESS)throw std::runtime_error("GpuAllocator: failed");
}
GpuAllocator::~GpuAllocator(){if(allocator_){vmaDestroyAllocator(allocator_);allocator_=VK_NULL_HANDLE;}}
GpuAllocator::GpuAllocator(GpuAllocator&&o)noexcept:allocator_(o.allocator_){o.allocator_=VK_NULL_HANDLE;}
GpuAllocator& GpuAllocator::operator=(GpuAllocator&&o)noexcept{if(this!=&o){if(allocator_)vmaDestroyAllocator(allocator_);allocator_=o.allocator_;o.allocator_=VK_NULL_HANDLE;}return*this;}
VkResult GpuAllocator::createImage(const VkImageCreateInfo* pCI,VkImage* pImage,void** pAlloc)const{VmaAllocationCreateInfo aci{};aci.usage=VMA_MEMORY_USAGE_AUTO;return vmaCreateImage(allocator_,pCI,&aci,pImage,reinterpret_cast<VmaAllocation*>(pAlloc),nullptr);}
void GpuAllocator::destroyImage(VkImage image,void* alloc)const{if(image&&alloc)vmaDestroyImage(allocator_,image,reinterpret_cast<VmaAllocation>(alloc));}
} }
#ifdef __clang__
#pragma clang diagnostic pop
#endif
