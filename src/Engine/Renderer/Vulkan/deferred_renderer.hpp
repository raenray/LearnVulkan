#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
namespace Engine { namespace Renderer {
class GpuAllocator; class ShaderManager;
class DeferredRenderer {
public:
    struct Att{VkImage im=VK_NULL_HANDLE; void* al=nullptr; VkImageView v=VK_NULL_HANDLE; VkFormat f=VK_FORMAT_UNDEFINED;};
    struct Frame{Att pos,norm,albedo,material,depth; VkFramebuffer fb=VK_NULL_HANDLE;};
    DeferredRenderer(); ~DeferredRenderer();
    DeferredRenderer(const DeferredRenderer&)=delete; DeferredRenderer& operator=(const DeferredRenderer&)=delete;
    void init(VkDevice d, VkPhysicalDevice p, GpuAllocator* a, ShaderManager* s, uint32_t w, uint32_t h, VkRenderPass swapchainRP);
    void resize(uint32_t w, uint32_t h);
    void destroy();
    VkRenderPass geoRP()const{return geoRP_;} VkFramebuffer geoFB(uint32_t i)const{return i<frames_.size()?frames_[i].fb:VK_NULL_HANDLE;}
    VkPipeline geoPipe()const{return geoPipe_;} VkPipelineLayout geoLayout()const{return geoLayout_;}
    VkPipeline lightPipe()const{return lightPipe_;} VkPipelineLayout lightLayout()const{return lightLayout_;}
    VkDescriptorSet lightDS(uint32_t i)const{return i<lightDS_.size()?lightDS_[i]:VK_NULL_HANDLE;}
    void beginLightingPass(VkCommandBuffer cb, uint32_t fi, VkRenderPass rp, VkFramebuffer fb, VkExtent2D e);
    VkExtent2D ext()const{return ex_;}
private:
    VkDevice d_=VK_NULL_HANDLE; VkPhysicalDevice p_=VK_NULL_HANDLE; GpuAllocator* a_=nullptr; ShaderManager* s_=nullptr; VkExtent2D ex_{};
    std::vector<Frame> frames_; VkRenderPass geoRP_=VK_NULL_HANDLE;
    VkPipelineLayout geoLayout_=VK_NULL_HANDLE; VkPipeline geoPipe_=VK_NULL_HANDLE;
    VkPipelineLayout lightLayout_=VK_NULL_HANDLE; VkPipeline lightPipe_=VK_NULL_HANDLE;
    VkDescriptorSetLayout lightDSL_=VK_NULL_HANDLE; VkDescriptorPool lightDP_=VK_NULL_HANDLE; std::vector<VkDescriptorSet> lightDS_;
    void createGeoPass(); void createLightPass(VkRenderPass rp); void createFrames(); void destroyFrames();
};
} }
