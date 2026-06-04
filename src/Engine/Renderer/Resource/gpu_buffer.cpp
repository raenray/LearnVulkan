#include "Engine/Renderer/Resource/gpu_buffer.hpp"
#include <vk_mem_alloc.h>
#include <cstring>
#include <stdexcept>
namespace Engine { namespace Renderer {
static inline VmaAllocator vma(void*h){return reinterpret_cast<VmaAllocator>(h);}
static inline VmaAllocation val(void*h){return reinterpret_cast<VmaAllocation>(h);}
GPUBuffer::GPUBuffer(void* vmaAlloc,const GPUBufferCreateInfo& ci):allocator_(vmaAlloc),sz_(ci.size){
    auto* a=vma(vmaAlloc);VmaAllocatorInfo ai;vmaGetAllocatorInfo(a,&ai);dev_=ai.device;
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bci.size=ci.size;bci.usage=ci.usage;
    VmaAllocationCreateInfo aci{};aci.usage=VMA_MEMORY_USAGE_AUTO;aci.requiredFlags=ci.requiredFlags;aci.preferredFlags=ci.preferredFlags;
    if(ci.requiredFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)aci.flags|=VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    VmaAllocationInfo alInfo;VkBuffer b;VmaAllocation al;
    if(vmaCreateBuffer(a,&bci,&aci,&b,&al,&alInfo)!=VK_SUCCESS)throw std::runtime_error(std::string("GPUBuffer: failed")+(ci.name?ci.name:""));
    buf_=b;allocation_=reinterpret_cast<void*>(al);if(ci.name)vmaSetAllocationName(a,al,ci.name);if(alInfo.pMappedData)mapped_=alInfo.pMappedData;}
GPUBuffer::~GPUBuffer(){if(mapped_){vmaUnmapMemory(vma(allocator_),val(allocation_));mapped_=nullptr;}if(buf_&&allocator_)vmaDestroyBuffer(vma(allocator_),buf_,val(allocation_));}
GPUBuffer::GPUBuffer(GPUBuffer&&o)noexcept:dev_(o.dev_),allocator_(o.allocator_),buf_(o.buf_),allocation_(o.allocation_),sz_(o.sz_),mapped_(o.mapped_){o.dev_=VK_NULL_HANDLE;o.allocator_=nullptr;o.buf_=VK_NULL_HANDLE;o.allocation_=nullptr;o.mapped_=nullptr;}
GPUBuffer& GPUBuffer::operator=(GPUBuffer&&o)noexcept{if(this!=&o){if(mapped_)vmaUnmapMemory(vma(allocator_),val(allocation_));if(buf_&&allocator_)vmaDestroyBuffer(vma(allocator_),buf_,val(allocation_));dev_=o.dev_;allocator_=o.allocator_;buf_=o.buf_;allocation_=o.allocation_;sz_=o.sz_;mapped_=o.mapped_;o.dev_=VK_NULL_HANDLE;o.allocator_=nullptr;o.buf_=VK_NULL_HANDLE;o.allocation_=nullptr;o.mapped_=nullptr;}return*this;}
void* GPUBuffer::map(){if(mapped_)return mapped_;void* p=nullptr;vmaMapMemory(vma(allocator_),val(allocation_),&p);mapped_=p;return p;}
void GPUBuffer::unmap(){if(!mapped_)return;vmaUnmapMemory(vma(allocator_),val(allocation_));mapped_=nullptr;}
void GPUBuffer::flush(VkDeviceSize o,VkDeviceSize s){vmaFlushAllocation(vma(allocator_),val(allocation_),o,s);}
void GPUBuffer::invalidate(VkDeviceSize o,VkDeviceSize s){vmaInvalidateAllocation(vma(allocator_),val(allocation_),o,s);}
void GPUBuffer::upload(VkCommandBuffer cmd,VkQueue q,const void* data,VkDeviceSize s,VkDeviceSize dstOff){
    VkDeviceSize cs=(s==VK_WHOLE_SIZE)?sz_:s;VkBuffer sb;VmaAllocation sa;
    VkBufferCreateInfo sbc{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};sbc.size=cs;sbc.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo sac{};sac.usage=VMA_MEMORY_USAGE_AUTO;sac.requiredFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;sac.flags|=VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    VmaAllocationInfo si;auto*A=vma(allocator_);vmaCreateBuffer(A,&sbc,&sac,&sb,&sa,&si);
    if(si.pMappedData)memcpy(si.pMappedData,data,(size_t)cs);else{void*p;vmaMapMemory(A,sa,&p);memcpy(p,data,(size_t)cs);vmaUnmapMemory(A,sa);}
    VkBufferCopy r{0,dstOff,cs};vkCmdCopyBuffer(cmd,sb,buf_,1,&r);
    VkSubmitInfo sbi{VK_STRUCTURE_TYPE_SUBMIT_INFO};sbi.commandBufferCount=1;sbi.pCommandBuffers=&cmd;vkQueueSubmit(q,1,&sbi,VK_NULL_HANDLE);vkQueueWaitIdle(q);vmaDestroyBuffer(A,sb,sa);}
} }
