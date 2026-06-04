#include "Engine/Platform/window.hpp"
#include "Engine/Platform/timer.hpp"
#include "Engine/Renderer/Vulkan/vulkan_context.hpp"
#include "Engine/Renderer/Vulkan/vulkan_swapchain.hpp"
#include "Engine/Renderer/Resource/gpu_allocator.hpp"
#include "Engine/Renderer/Resource/gpu_buffer.hpp"
#include "Engine/Renderer/Shader/shader_manager.hpp"
#include "Engine/Renderer/Vulkan/deferred_renderer.hpp"
#include "Engine/Editor/editor.hpp"
#include <GLFW/glfw3.h>
#define GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <algorithm>

constexpr uint32_t WW=800,HH=600; constexpr int MAX_FIF=2;
struct Vertex{glm::vec3 pos, normal;};
static auto MakeSphere(int lat,int lon,float r=1){
    std::vector<Vertex>v;std::vector<uint32_t>idx;
    for(int i=0;i<=lat;++i){float t=glm::pi<float>()*i/lat,st=sin(t),ct=cos(t);for(int j=0;j<=lon;++j){float p=2*glm::pi<float>()*j/lon,sp=sin(p),cp=cos(p);glm::vec3 n(st*cp,ct,st*sp);v.push_back({n*r,n});}}
    for(int i=0;i<lat;++i)for(int j=0;j<lon;++j){uint32_t a=i*(lon+1)+j,b=a+lon+1,c=a+1,d=b+1;idx.insert(idx.end(),{a,b,c,c,b,d});}
    return std::make_pair(v,idx);}

struct Camera{glm::vec3 pos{0,2.5f,6};float yaw=-90,pitch=-15;int lmx=0,lmy=0;
    void upd(float dt,GLFWwindow*w){float s=5*dt,ry=glm::radians(yaw),rp=glm::radians(pitch);
        glm::vec3 f{cos(ry)*cos(rp),sin(rp),sin(ry)*cos(rp)},r=glm::normalize(glm::cross(f,{0,1,0}));
        if(glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS)pos+=f*s;if(glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS)pos-=f*s;
        if(glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS)pos-=r*s;if(glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS)pos+=r*s;
        if(glfwGetKey(w,GLFW_KEY_Q)==GLFW_PRESS)pos.y-=s;if(glfwGetKey(w,GLFW_KEY_E)==GLFW_PRESS)pos.y+=s;
        if(glfwGetKey(w,GLFW_KEY_R)==GLFW_PRESS){pos={0,2.5f,6};yaw=-90;pitch=-15;}}
    glm::vec3 fwd()const{float ry=glm::radians(yaw),rp=glm::radians(pitch);return{cos(ry)*cos(rp),sin(rp),sin(ry)*cos(rp)};}};

class App{public:void run(){init();loop();cleanup();}
private:
    std::unique_ptr<Engine::Platform::Window> w_;
    std::unique_ptr<Engine::Renderer::VulkanContext> ctx_;
    Engine::Renderer::VulkanSwapchain* sw_=nullptr;
    std::unique_ptr<Engine::Renderer::GpuAllocator> alloc_;
    std::unique_ptr<Engine::Renderer::ShaderManager> sm_;
    std::unique_ptr<Engine::Renderer::DeferredRenderer> def_;
    std::unique_ptr<Engine::Renderer::GpuBuffer> vb_,ib_;
    VkCommandPool cp_; std::vector<VkCommandBuffer> cbs_; std::vector<VkSemaphore> sa_,sr_; std::vector<VkFence> fences_; uint32_t fi_=0;
    std::vector<Vertex> sv_; std::vector<uint32_t> si_; Engine::Platform::Timer timer_; int frame_=0; Camera cam_; float angle_=0;
    Engine::Editor::EditorUI editor_;
    VkDevice dev()const{return ctx_->device();}

    void init(){
        w_=std::make_unique<Engine::Platform::Window>(WW,HH,"LearnVulkan Editor");
        ctx_=std::make_unique<Engine::Renderer::VulkanContext>(w_->native());
        sw_=&ctx_->swapchain();
        alloc_=std::make_unique<Engine::Renderer::GpuAllocator>(ctx_->instance(),ctx_->physicalDevice(),dev());
        sm_=std::make_unique<Engine::Renderer::ShaderManager>(dev());
        VkCommandPoolCreateInfo cci{};cci.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;cci.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;cci.queueFamilyIndex=ctx_->graphicsQueueFamily();vkCreateCommandPool(dev(),&cci,nullptr,&cp_);
        def_=std::make_unique<Engine::Renderer::DeferredRenderer>();
        def_->init(dev(),ctx_->physicalDevice(),alloc_.get(),sm_.get(),WW,HH,sw_->renderPass());
        auto[v,idx]=MakeSphere(32,64);sv_=std::move(v);si_=std::move(idx);
        void* vma=alloc_->handle();VkQueue q=ctx_->graphicsQueue();
        auto up=[&](Engine::Renderer::GpuBuffer& d,const void* p,VkDeviceSize s){
            Engine::Renderer::GpuBufferDesc sc{};sc.size=s;sc.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;sc.requiredFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            Engine::Renderer::GpuBuffer st(vma,sc);memcpy(st.map(),p,(size_t)s);
            VkCommandBuffer cb;VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=cp_;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;vkAllocateCommandBuffers(dev(),&ai,&cb);
            VkCommandBufferBeginInfo bi{};bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;vkBeginCommandBuffer(cb,&bi);VkBufferCopy r{0,0,s};vkCmdCopyBuffer(cb,st.buffer(),d.buffer(),1,&r);vkEndCommandBuffer(cb);
            VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.commandBufferCount=1;si.pCommandBuffers=&cb;vkQueueSubmit(q,1,&si,VK_NULL_HANDLE);vkQueueWaitIdle(q);vkFreeCommandBuffers(dev(),cp_,1,&cb);};
        VkDeviceSize vsz=sizeof(Vertex)*sv_.size();Engine::Renderer::GpuBufferDesc vbci{};vbci.size=vsz;vbci.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;vb_=std::make_unique<Engine::Renderer::GpuBuffer>(vma,vbci);up(*vb_,sv_.data(),vsz);
        VkDeviceSize isz=sizeof(uint32_t)*si_.size();Engine::Renderer::GpuBufferDesc ibci{};ibci.size=isz;ibci.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT;ib_=std::make_unique<Engine::Renderer::GpuBuffer>(vma,ibci);up(*ib_,si_.data(),isz);
        sa_.resize(MAX_FIF);sr_.resize(MAX_FIF);fences_.resize(MAX_FIF);cbs_.resize(MAX_FIF);VkSemaphoreCreateInfo ss{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fi.flags=VK_FENCE_CREATE_SIGNALED_BIT;
        for(int i=0;i<MAX_FIF;i++){vkCreateSemaphore(dev(),&ss,nullptr,&sa_[i]);vkCreateSemaphore(dev(),&ss,nullptr,&sr_[i]);vkCreateFence(dev(),&fi,nullptr,&fences_[i]);}
        VkCommandBufferAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ai.commandPool=cp_;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=MAX_FIF;vkAllocateCommandBuffers(dev(),&ai,cbs_.data());
        editor_.init(w_->native(),ctx_->instance(),ctx_->physicalDevice(),dev(),ctx_->graphicsQueueFamily(),ctx_->graphicsQueue(),sw_->renderPass(),3);
        editor_.log(Engine::Editor::ConsoleEntry::Level::Info,"Editor initialized");
    }

    void loop(){while(!w_->shouldClose()&&!editor_.wantsQuit()){
        w_->pollEvents();bool camActive=editor_.cameraActive();
        if(glfwGetKey(w_->native(),GLFW_KEY_ESCAPE)==GLFW_PRESS){if(camActive){editor_.setCameraActive(false);glfwSetInputMode(w_->native(),GLFW_CURSOR,GLFW_CURSOR_NORMAL);}}
        if(camActive){double mx,my;glfwGetCursorPos(w_->native(),&mx,&my);int cx=(int)mx,cy=(int)my;if(frame_>0){cam_.yaw+=(cx-cam_.lmx)*0.15f;cam_.pitch+=(cy-cam_.lmy)*0.15f;cam_.pitch=std::clamp(cam_.pitch,-89.0f,89.0f);}cam_.lmx=cx;cam_.lmy=cy;}
        timer_.tick();float dt=timer_.deltaTime();if(camActive)cam_.upd(dt,w_->native());angle_+=0.8f*dt;
        int fw=0,fh=0;w_->getFramebufferSize(&fw,&fh);bool nr=(fw>0&&fh>0&&(fw!=(int)sw_->extent().width||fh!=(int)sw_->extent().height));
        if(nr){resize();continue;}
        editor_.setFrameTime(dt*1000);editor_.setCameraPos(cam_.pos.x,cam_.pos.y,cam_.pos.z);editor_.beginFrame();
        if(editor_.cameraActive()!=camActive){camActive=editor_.cameraActive();if(camActive){glfwSetInputMode(w_->native(),GLFW_CURSOR,GLFW_CURSOR_DISABLED);double mx,my;glfwGetCursorPos(w_->native(),&mx,&my);cam_.lmx=(int)mx;cam_.lmy=(int)my;}else{glfwSetInputMode(w_->native(),GLFW_CURSOR,GLFW_CURSOR_NORMAL);}}
        drawFrame();frame_++;}vkDeviceWaitIdle(dev());}

    void drawFrame(){vkWaitForFences(dev(),1,&fences_[fi_],VK_TRUE,UINT64_MAX);uint32_t ix;vkResetFences(dev(),1,&fences_[fi_]);
        VkResult r;for(int tries=0;tries<5;++tries){r=vkAcquireNextImageKHR(dev(),sw_->swapchain(),UINT64_MAX,sa_[fi_],VK_NULL_HANDLE,&ix);if(r==VK_SUCCESS||r==VK_SUBOPTIMAL_KHR)break;if(r==VK_ERROR_OUT_OF_DATE_KHR){resize();return;}vkDeviceWaitIdle(dev());}
        vkResetCommandBuffer(cbs_[fi_],0);VkCommandBuffer cb=cbs_[fi_];VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};vkBeginCommandBuffer(cb,&bi);
        float aspect=(float)sw_->extent().width/(float)sw_->extent().height;glm::mat4 view=glm::lookAt(cam_.pos,cam_.pos+cam_.fwd(),{0,1,0}),proj=glm::perspective(glm::radians(60.0f),aspect,0.1f,100.0f);proj[1][1]*=-1;glm::mat4 model=glm::rotate(glm::mat4(1),angle_,glm::vec3(0,1,0));

        VkClearValue gclr[5]{};gclr[0].color=gclr[1].color=gclr[2].color=gclr[3].color={{0,0,0,0}};gclr[4].depthStencil={1,0};
        VkRenderPassBeginInfo grp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};grp.renderPass=def_->geoRP();grp.framebuffer=def_->geoFB(ix%3);grp.renderArea.extent=sw_->extent();grp.clearValueCount=5;grp.pClearValues=gclr;vkCmdBeginRenderPass(cb,&grp,VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,def_->geoPipe());
        VkViewport vp{0,(float)sw_->extent().height,(float)sw_->extent().width,-(float)sw_->extent().height,0,1};vkCmdSetViewport(cb,0,1,&vp);VkRect2D sc{{0,0},sw_->extent()};vkCmdSetScissor(cb,0,1,&sc);
        VkBuffer vbo[]={vb_->buffer()};VkDeviceSize off[]={0};vkCmdBindVertexBuffers(cb,0,1,vbo,off);vkCmdBindIndexBuffer(cb,ib_->buffer(),0,VK_INDEX_TYPE_UINT32);
        struct{glm::mat4 mvp,mdl;}gpc;gpc.mdl=model;gpc.mvp=proj*view*model;vkCmdPushConstants(cb,def_->geoLayout(),VK_SHADER_STAGE_VERTEX_BIT,0,128,&gpc);
        vkCmdDrawIndexed(cb,(uint32_t)si_.size(),1,0,0,0);vkCmdEndRenderPass(cb);

        def_->beginLightingPass(cb,(uint32_t)(ix%3),sw_->renderPass(),sw_->framebuffers()[ix],sw_->extent());
        glm::vec3 lightDir=glm::normalize(glm::vec3(-0.5f,-1.0f,0.3f));
        struct{glm::vec4 cp;glm::vec4 ld;glm::vec4 lcl;glm::mat4 snull;}lpc;lpc.cp=glm::vec4(cam_.pos,0);lpc.ld=glm::vec4(lightDir,0);lpc.lcl=glm::vec4(1.0f,0.95f,0.85f,1.5f);lpc.snull=glm::mat4(1);
        vkCmdPushConstants(cb,def_->lightLayout(),VK_SHADER_STAGE_FRAGMENT_BIT,0,128,&lpc);vkCmdDraw(cb,3,1,0,0);
        editor_.endFrame(cb);vkCmdEndRenderPass(cb);vkEndCommandBuffer(cb);

        VkSemaphore ws[]={sa_[fi_]};VkPipelineStageFlags wf[]={VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.waitSemaphoreCount=1;si.pWaitSemaphores=ws;si.pWaitDstStageMask=wf;si.commandBufferCount=1;si.pCommandBuffers=&cb;si.signalSemaphoreCount=1;si.pSignalSemaphores=&sr_[fi_];vkQueueSubmit(ctx_->graphicsQueue(),1,&si,fences_[fi_]);
        VkSwapchainKHR swc=sw_->swapchain();VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,nullptr,1,&sr_[fi_],1,&swc,&ix,nullptr};r=vkQueuePresentKHR(ctx_->graphicsQueue(),&pi);
        if(r==VK_ERROR_OUT_OF_DATE_KHR||r==VK_SUBOPTIMAL_KHR){}fi_=(fi_+1)%MAX_FIF;}

    void resize(){int w=0,h=0;w_->getFramebufferSize(&w,&h);while(w==0||h==0){w_->getFramebufferSize(&w,&h);glfwWaitEvents();}vkDeviceWaitIdle(dev());
        def_.reset();ctx_->recreateSwapchain(w,h);sw_=&ctx_->swapchain();def_=std::make_unique<Engine::Renderer::DeferredRenderer>();def_->init(dev(),ctx_->physicalDevice(),alloc_.get(),sm_.get(),w,h,sw_->renderPass());editor_.onSwapchainResize(sw_->renderPass());}
    void cleanup(){vkQueueWaitIdle(ctx_->graphicsQueue());vkDeviceWaitIdle(dev());for(int i=0;i<MAX_FIF;i++){if(fences_[i])vkWaitForFences(dev(),1,&fences_[i],VK_TRUE,UINT64_MAX);vkDestroySemaphore(dev(),sr_[i],nullptr);vkDestroySemaphore(dev(),sa_[i],nullptr);vkDestroyFence(dev(),fences_[i],nullptr);}vkDestroyCommandPool(dev(),cp_,nullptr);cp_=VK_NULL_HANDLE;}
};
int main(){App app;try{app.run();}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}return 0;}
