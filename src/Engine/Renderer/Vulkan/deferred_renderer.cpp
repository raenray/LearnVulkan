#include "Engine/Renderer/Vulkan/deferred_renderer.hpp"
#include "Engine/Renderer/Resource/gpu_allocator.hpp"
#include "Engine/Renderer/Shader/shader_manager.hpp"
#include <stdexcept>

namespace Engine
{
namespace Renderer
{

static VkResult mkImg(GpuAllocator& a, uint32_t w, uint32_t h, VkFormat f, VkImageUsageFlags u, VkImage& im, void*& al)
{
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = f;
    ci.extent = {w, h, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = u;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    return a.createImage(&ci, &im, &al);
}

static void mkView(VkDevice d, VkImage im, VkFormat f, VkImageAspectFlags a, VkImageView& v)
{
    VkImageViewCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ci.image = im;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format = f;
    ci.subresourceRange = {a, 0, 1, 0, 1};
    vkCreateImageView(d, &ci, nullptr, &v);
}

static void delAtt(VkDevice d, GpuAllocator& a, DeferredRenderer::Att& x)
{
    if (x.v)
    {
        vkDestroyImageView(d, x.v, nullptr);
        x.v = VK_NULL_HANDLE;
    }
    a.destroyImage(x.im, x.al);
    x.im = VK_NULL_HANDLE;
    x.al = nullptr;
}

static VkPipeline MakePipe(VkDevice d,
                           VkShaderModule vs,
                           VkShaderModule fs,
                           VkVertexInputBindingDescription* bd,
                           uint32_t bc,
                           VkVertexInputAttributeDescription* ad,
                           uint32_t ac,
                           VkPrimitiveTopology topo,
                           uint32_t cc,
                           VkBool32 dt,
                           VkBool32 dw,
                           VkPipelineLayout ly,
                           VkRenderPass rp,
                           uint32_t sub,
                           VkCullModeFlags cull)
{
    VkPipelineShaderStageCreateInfo ss[2]{};
    ss[0].sType = ss[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    ss[0].module = vs;
    ss[0].pName = "main";
    ss[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ss[1].module = fs;
    ss[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = bc;
    vi.pVertexBindingDescriptions = bd;
    vi.vertexAttributeDescriptionCount = ac;
    vi.pVertexAttributeDescriptions = ad;
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = topo;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = cull;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = dt;
    ds.depthWriteEnable = dw;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;
    std::vector<VkPipelineColorBlendAttachmentState> cbas(cc);
    for (auto& c : cbas)
        c.colorWriteMask = 0xF;
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = cc;
    cb.pAttachments = cbas.data();
    VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 2;
    dy.pDynamicStates = dyn;
    VkGraphicsPipelineCreateInfo gci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gci.stageCount = 2;
    gci.pStages = ss;
    gci.pVertexInputState = &vi;
    gci.pInputAssemblyState = &ia;
    gci.pViewportState = &vp;
    gci.pRasterizationState = &rs;
    gci.pMultisampleState = &ms;
    gci.pDepthStencilState = &ds;
    gci.pColorBlendState = &cb;
    gci.pDynamicState = &dy;
    gci.layout = ly;
    gci.renderPass = rp;
    gci.subpass = sub;
    VkPipeline pipe;
    vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &gci, nullptr, &pipe);
    return pipe;
}

DeferredRenderer::DeferredRenderer() = default;

DeferredRenderer::~DeferredRenderer()
{
    destroy();
}

void DeferredRenderer::init(VkDevice d, VkPhysicalDevice p, GpuAllocator* a, ShaderManager* s, uint32_t w, uint32_t h, VkRenderPass rp)
{
    d_ = d;
    p_ = p;
    a_ = a;
    s_ = s;
    ex_ = {w, h};
    createGeoPass();
    createLightPass(rp);
    frames_.resize(3);
    createFrames();
}

void DeferredRenderer::resize(uint32_t w, uint32_t h)
{
    ex_ = {w, h};
    destroyFrames();
    createFrames();
}

void DeferredRenderer::destroy()
{
    destroyFrames();
    for (auto& ds : lightDS_)
        if (ds)
            vkFreeDescriptorSets(d_, lightDP_, 1, &ds);
    lightDS_.clear();
    if (lightDP_)
    {
        vkDestroyDescriptorPool(d_, lightDP_, nullptr);
        lightDP_ = VK_NULL_HANDLE;
    }
    if (lightDSL_)
    {
        vkDestroyDescriptorSetLayout(d_, lightDSL_, nullptr);
        lightDSL_ = VK_NULL_HANDLE;
    }
    if (geoPipe_)
    {
        vkDestroyPipeline(d_, geoPipe_, nullptr);
    }
    if (lightPipe_)
    {
        vkDestroyPipeline(d_, lightPipe_, nullptr);
    }
    if (geoLayout_)
    {
        vkDestroyPipelineLayout(d_, geoLayout_, nullptr);
    }
    if (lightLayout_)
    {
        vkDestroyPipelineLayout(d_, lightLayout_, nullptr);
    }
    if (geoRP_)
    {
        vkDestroyRenderPass(d_, geoRP_, nullptr);
    }
}

void DeferredRenderer::createGeoPass()
{
    auto gv = s_->compileFile(SHADERS_DIR "/gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT);
    auto gf = s_->compileFile(SHADERS_DIR "/gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPushConstantRange pc{VK_SHADER_STAGE_VERTEX_BIT, 0, 128};
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &pc;
    vkCreatePipelineLayout(d_, &pl, nullptr, &geoLayout_);
    VkAttachmentDescription Att[5]{};
    Att[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    Att[1].format = VK_FORMAT_R16G16B16A16_SNORM;
    Att[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    Att[3].format = VK_FORMAT_R8G8_UNORM;
    Att[4].format = VK_FORMAT_D32_SFLOAT;
    for (int i = 0; i < 5; i++)
    {
        Att[i].samples = VK_SAMPLE_COUNT_1_BIT;
        Att[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        Att[i].storeOp = (i < 4) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        Att[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Att[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        Att[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        Att[i].finalLayout = (i < 4) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference cr[4] = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                                   {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                                   {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                                   {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
    VkAttachmentReference dr{4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 4;
    sp.pColorAttachments = cr;
    sp.pDepthStencilAttachment = &dr;
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rci.attachmentCount = 5;
    rci.pAttachments = Att;
    rci.subpassCount = 1;
    rci.pSubpasses = &sp;
    rci.dependencyCount = 1;
    rci.pDependencies = &dep;
    vkCreateRenderPass(d_, &rci, nullptr, &geoRP_);
    VkVertexInputBindingDescription bd{0, 24, VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription ads[2] = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12}};
    geoPipe_ =
        MakePipe(d_, s_->getModule(gv), s_->getModule(gf), &bd, 1, ads, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 4, VK_TRUE, VK_TRUE, geoLayout_, geoRP_, 0, VK_CULL_MODE_BACK_BIT);
}

void DeferredRenderer::createLightPass(VkRenderPass rp)
{
    auto lv = s_->compileFile(SHADERS_DIR "/lighting.vert", VK_SHADER_STAGE_VERTEX_BIT);
    auto lf = s_->compileFile(SHADERS_DIR "/lighting.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding b[5]{};
    for (int i = 0; i < 4; i++)
    {
        b[i].binding = i;
        b[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        b[i].descriptorCount = 1;
        b[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    b[4].binding = 4;
    b[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    b[4].descriptorCount = 1;
    b[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo dsl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dsl.bindingCount = 5;
    dsl.pBindings = b;
    vkCreateDescriptorSetLayout(d_, &dsl, nullptr, &lightDSL_);
    VkPushConstantRange pc{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 128};
    VkDescriptorSetLayout sl[] = {lightDSL_};
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &pc;
    pl.setLayoutCount = 1;
    pl.pSetLayouts = sl;
    vkCreatePipelineLayout(d_, &pl, nullptr, &lightLayout_);
    VkDescriptorPoolSize ps[] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * 3}};
    VkDescriptorPoolCreateInfo dpci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpci.maxSets = 3;
    dpci.poolSizeCount = 1;
    dpci.pPoolSizes = ps;
    vkCreateDescriptorPool(d_, &dpci, nullptr, &lightDP_);
    lightPipe_ =
        MakePipe(d_, s_->getModule(lv), s_->getModule(lf), nullptr, 0, nullptr, 0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 1, VK_FALSE, VK_FALSE, lightLayout_, rp, 0, VK_CULL_MODE_NONE);
}

void DeferredRenderer::beginLightingPass(VkCommandBuffer cb, uint32_t idx, VkRenderPass rp, VkFramebuffer fb, VkExtent2D e)
{
    VkClearValue cv{};
    cv.color = {{0.02f, 0.02f, 0.05f, 1.0f}};
    VkRenderPassBeginInfo r{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    r.renderPass = rp;
    r.framebuffer = fb;
    r.renderArea.extent = e;
    r.clearValueCount = 1;
    r.pClearValues = &cv;
    vkCmdBeginRenderPass(cb, &r, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, lightPipe_);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, lightLayout_, 0, 1, &lightDS_[idx], 0, nullptr);
    VkViewport vp{0, (float) e.height, (float) e.width, -(float) e.height, 0, 1};
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D sc{{0, 0}, e};
    vkCmdSetScissor(cb, 0, 1, &sc);
}

void DeferredRenderer::createFrames()
{
    VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sci.magFilter = VK_FILTER_LINEAR;
    sci.minFilter = VK_FILTER_LINEAR;
    sci.addressModeU = sci.addressModeV = sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSampler sampler;
    vkCreateSampler(d_, &sci, nullptr, &sampler);
    lightDS_.resize(frames_.size());
    VkDescriptorSetAllocateInfo dsai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dsai.descriptorPool = lightDP_;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &lightDSL_;
    for (size_t i = 0; i < frames_.size(); i++)
    {
        auto& f = frames_[i];
        VkImageUsageFlags cu = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        f.pos.f = VK_FORMAT_R16G16B16A16_SFLOAT;
        mkImg(*a_, ex_.width, ex_.height, f.pos.f, cu, f.pos.im, f.pos.al);
        mkView(d_, f.pos.im, f.pos.f, VK_IMAGE_ASPECT_COLOR_BIT, f.pos.v);
        f.norm.f = VK_FORMAT_R16G16B16A16_SNORM;
        mkImg(*a_, ex_.width, ex_.height, f.norm.f, cu, f.norm.im, f.norm.al);
        mkView(d_, f.norm.im, f.norm.f, VK_IMAGE_ASPECT_COLOR_BIT, f.norm.v);
        f.albedo.f = VK_FORMAT_R8G8B8A8_UNORM;
        mkImg(*a_, ex_.width, ex_.height, f.albedo.f, cu, f.albedo.im, f.albedo.al);
        mkView(d_, f.albedo.im, f.albedo.f, VK_IMAGE_ASPECT_COLOR_BIT, f.albedo.v);
        f.material.f = VK_FORMAT_R8G8_UNORM;
        mkImg(*a_, ex_.width, ex_.height, f.material.f, cu, f.material.im, f.material.al);
        mkView(d_, f.material.im, f.material.f, VK_IMAGE_ASPECT_COLOR_BIT, f.material.v);
        f.depth.f = VK_FORMAT_D32_SFLOAT;
        mkImg(*a_,
              ex_.width,
              ex_.height,
              f.depth.f,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              f.depth.im,
              f.depth.al);
        mkView(d_, f.depth.im, f.depth.f, VK_IMAGE_ASPECT_DEPTH_BIT, f.depth.v);
        VkImageView atts[] = {f.pos.v, f.norm.v, f.albedo.v, f.material.v, f.depth.v};
        VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fci.renderPass = geoRP_;
        fci.attachmentCount = 5;
        fci.pAttachments = atts;
        fci.width = ex_.width;
        fci.height = ex_.height;
        fci.layers = 1;
        vkCreateFramebuffer(d_, &fci, nullptr, &f.fb);
        vkAllocateDescriptorSets(d_, &dsai, &lightDS_[i]);
        VkDescriptorImageInfo dii[5]{};
        for (int j = 0; j < 5; j++)
        {
            dii[j].sampler = sampler;
            dii[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        dii[0].imageView = f.pos.v;
        dii[1].imageView = f.norm.v;
        dii[2].imageView = f.albedo.v;
        dii[3].imageView = f.material.v;
        dii[4].imageView = f.depth.v;
        dii[4].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        VkWriteDescriptorSet w[5]{};
        for (int j = 0; j < 5; j++)
        {
            w[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w[j].dstSet = lightDS_[i];
            w[j].dstBinding = j;
            w[j].descriptorCount = 1;
            w[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            w[j].pImageInfo = &dii[j];
        }
        vkUpdateDescriptorSets(d_, 5, w, 0, nullptr);
    }
    vkDestroySampler(d_, sampler, nullptr);
}

void DeferredRenderer::destroyFrames()
{
    for (auto& f : frames_)
    {
        if (f.fb)
        {
            vkDestroyFramebuffer(d_, f.fb, nullptr);
        }
        delAtt(d_, *a_, f.pos);
        delAtt(d_, *a_, f.norm);
        delAtt(d_, *a_, f.albedo);
        delAtt(d_, *a_, f.material);
        delAtt(d_, *a_, f.depth);
    }
    frames_.clear();
}
} // namespace Renderer
} // namespace Engine
