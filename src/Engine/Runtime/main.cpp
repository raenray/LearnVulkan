#include "Engine/Platform/window.hpp"
#include "Engine/Platform/timer.hpp"
#include "Engine/Renderer/Vulkan/vulkan_context.hpp"
#include "Engine/Renderer/Vulkan/vulkan_swapchain.hpp"
#include "Engine/Renderer/Resource/gpu_allocator.hpp"
#include "Engine/Renderer/Mesh/mesh.hpp"
#include "Engine/Renderer/Mesh/mesh_cache.hpp"
#include "Engine/Renderer/Mesh/scene_renderer.hpp"
#include "Engine/Renderer/Shader/shader_manager.hpp"
#include "Engine/Renderer/Vulkan/deferred_renderer.hpp"
#include "Engine/Scene/scene.hpp"
#include "Engine/Game/game.hpp"
#include "Engine/Editor/editor.hpp"
#include <GLFW/glfw3.h>
#define GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>
#include <fstream>

constexpr int MAX_FIF = 2;

static uint32_t GetMonitorWidth()
{
    GLFWmonitor* m = glfwGetPrimaryMonitor();
    if (!m)
        return 800;
    const GLFWvidmode* v = glfwGetVideoMode(m);
    return v ? (uint32_t) (v->width * 0.8f) : 800;
}

static uint32_t GetMonitorHeight()
{
    GLFWmonitor* m = glfwGetPrimaryMonitor();
    if (!m)
        return 600;
    const GLFWvidmode* v = glfwGetVideoMode(m);
    return v ? (uint32_t) (v->height * 0.8f) : 600;
}

// ── settings persistence ─────────────────────────────────────────────

static bool LoadSettings(int& w, int& h)
{
    std::ifstream f("settings.cfg");
    if (!f)
        return false;
    f >> w >> h;
    return f && w > 0 && h > 0;
}

static void SaveSettings(int w, int h)
{
    std::ofstream f("settings.cfg");
    if (f)
        f << w << ' ' << h;
}

class App
{
public:
    void run()
    {
        init();
        loop();
        cleanup();
    }

private:
    std::unique_ptr<Engine::Platform::Window> w_;
    std::unique_ptr<Engine::Renderer::VulkanContext> ctx_;
    Engine::Renderer::VulkanSwapchain* sw_ = nullptr;
    std::unique_ptr<Engine::Renderer::GpuAllocator> alloc_;
    std::unique_ptr<Engine::Renderer::ShaderManager> sm_;
    std::unique_ptr<Engine::Renderer::DeferredRenderer> def_;
    VkCommandPool cp_;
    std::vector<VkCommandBuffer> cbs_;
    std::vector<VkSemaphore> sa_, sr_;
    std::vector<VkFence> fences_;
    uint32_t fi_ = 0;
    Engine::Platform::Timer timer_;
    int frame_ = 0;
    Engine::Editor::EditorUI editor_;

    // ── Game state ───────────────────────────────────────────────────
    Engine::Scene::Scene scene_;
    Engine::Renderer::MeshCache meshCache_;
    Engine::Renderer::SceneRenderer sceneRenderer_;
    Engine::Game game_;

    VkDevice dev() const { return ctx_->device(); }

    void init()
    {
        uint32_t WW = GetMonitorWidth(), HH = GetMonitorHeight();
        int sw = 0, sh = 0;
        if (LoadSettings(sw, sh))
        {
            WW = (uint32_t) sw;
            HH = (uint32_t) sh;
        }
        w_ = std::make_unique<Engine::Platform::Window>(WW, HH, "LearnVulkan Editor");
        ctx_ = std::make_unique<Engine::Renderer::VulkanContext>(w_->native());
        sw_ = &ctx_->swapchain();
        alloc_ = std::make_unique<Engine::Renderer::GpuAllocator>(ctx_->instance(), ctx_->physicalDevice(), dev());
        sm_ = std::make_unique<Engine::Renderer::ShaderManager>(dev());

        VkCommandPoolCreateInfo cci{};
        cci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cci.queueFamilyIndex = ctx_->graphicsQueueFamily();
        vkCreateCommandPool(dev(), &cci, nullptr, &cp_);

        def_ = std::make_unique<Engine::Renderer::DeferredRenderer>();
        def_->init(dev(), ctx_->physicalDevice(), alloc_.get(), sm_.get(), WW, HH, sw_->renderPass());

        // ── Create sphere mesh in cache ──────────────────────────────
        {
            namespace R = Engine::Renderer;
            int lat = 32, lon = 64;
            float r = 1.0f;
            std::vector<R::Vertex> v;
            std::vector<uint32_t> idx;
            for (int i = 0; i <= lat; ++i)
            {
                float t = glm::pi<float>() * i / lat, st = sin(t), ct = cos(t);
                for (int j = 0; j <= lon; ++j)
                {
                    float p = 2 * glm::pi<float>() * j / lon, sp = sin(p), cp = cos(p);
                    glm::vec3 n(st * cp, ct, st * sp);
                    v.push_back({n * r, n});
                }
            }
            for (int i = 0; i < lat; ++i)
                for (int j = 0; j < lon; ++j)
                {
                    uint32_t a = i * (lon + 1) + j, b = a + lon + 1, c = a + 1, d = b + 1;
                    idx.insert(idx.end(), {a, b, c, c, b, d});
                }
            meshCache_.createMesh("sphere", v, idx, alloc_->handle(), dev(), cp_, ctx_->graphicsQueue());
        }

        // ── Init game world ──────────────────────────────────────────
        game_.init(scene_, meshCache_);

        // ── Sync fences / semaphores ─────────────────────────────────
        sa_.resize(MAX_FIF);
        sr_.resize(MAX_FIF);
        fences_.resize(MAX_FIF);
        cbs_.resize(MAX_FIF);
        VkSemaphoreCreateInfo ss{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (int i = 0; i < MAX_FIF; i++)
        {
            vkCreateSemaphore(dev(), &ss, nullptr, &sa_[i]);
            vkCreateSemaphore(dev(), &ss, nullptr, &sr_[i]);
            vkCreateFence(dev(), &fi, nullptr, &fences_[i]);
        }
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = cp_;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = MAX_FIF;
        vkAllocateCommandBuffers(dev(), &ai, cbs_.data());

        editor_.init(w_->native(),
                     ctx_->instance(),
                     ctx_->physicalDevice(),
                     dev(),
                     ctx_->graphicsQueueFamily(),
                     ctx_->graphicsQueue(),
                     sw_->renderPass(),
                     3);
        editor_.log(Engine::Editor::ConsoleEntry::Level::Info, "Editor initialized");
        int ifw = 0, ifh = 0;
        w_->getFramebufferSize(&ifw, &ifh);
        editor_.setFramebufferSize(ifw, ifh);
    }

    void loop()
    {
        while (!w_->shouldClose() && !editor_.wantsQuit())
        {
            w_->pollEvents();
            // ── Editor-requested window resize ──────────────────────
            int nrw = 0, nrh = 0;
            if (editor_.pollResizeRequest(nrw, nrh))
            {
                w_->resizeWindow(nrw, nrh);
                editor_.clearResizeRequest();
                SaveSettings(nrw, nrh);
                continue;
            }

            // ── Camera input (only when editor says so) ────────────
            bool camActive = editor_.cameraActive();
            if (glfwGetKey(w_->native(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                if (camActive)
                {
                    editor_.setCameraActive(false);
                    glfwSetInputMode(w_->native(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }
            if (camActive)
            {
                double mx, my;
                glfwGetCursorPos(w_->native(), &mx, &my);
                int cx = (int) mx, cy = (int) my;
                if (frame_ > 0)
                {
                    game_.cameraYaw += (cx - game_.lastMouseX) * 0.15f;
                    game_.cameraPitch += (cy - game_.lastMouseY) * 0.15f;
                    game_.cameraPitch = std::clamp(game_.cameraPitch, -89.0f, 89.0f);
                }
                game_.lastMouseX = cx;
                game_.lastMouseY = cy;
            }

            timer_.tick();
            float dt = timer_.deltaTime();
            game_.update(camActive ? dt : 0.0f, w_->native());

            int fw = 0, fh = 0;
            w_->getFramebufferSize(&fw, &fh);
            editor_.setFramebufferSize(fw, fh);
            bool nr = (fw > 0 && fh > 0 && (fw != (int) sw_->extent().width || fh != (int) sw_->extent().height));
            if (nr)
            {
                resize();
                continue;
            }

            editor_.setFrameTime(dt * 1000);
            auto cp = game_.cameraPosition();
            editor_.setCameraPos(cp.x, cp.y, cp.z);
            editor_.beginFrame();

            if (editor_.cameraActive() != camActive)
            {
                camActive = editor_.cameraActive();
                if (camActive)
                {
                    glfwSetInputMode(w_->native(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    double mx, my;
                    glfwGetCursorPos(w_->native(), &mx, &my);
                    game_.lastMouseX = (int) mx;
                    game_.lastMouseY = (int) my;
                }
                else
                {
                    glfwSetInputMode(w_->native(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }
            drawFrame();
            frame_++;
        }
        vkDeviceWaitIdle(dev());
    }

    void drawFrame()
    {
        vkWaitForFences(dev(), 1, &fences_[fi_], VK_TRUE, UINT64_MAX);
        uint32_t ix;
        vkResetFences(dev(), 1, &fences_[fi_]);
        VkResult r;
        for (int tries = 0; tries < 5; ++tries)
        {
            r = vkAcquireNextImageKHR(dev(), sw_->swapchain(), UINT64_MAX, sa_[fi_], VK_NULL_HANDLE, &ix);
            if (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR)
                break;
            if (r == VK_ERROR_OUT_OF_DATE_KHR)
            {
                resize();
                return;
            }
            vkDeviceWaitIdle(dev());
        }
        vkResetCommandBuffer(cbs_[fi_], 0);
        VkCommandBuffer cb = cbs_[fi_];
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(cb, &bi);

        // ── Geometry pass ────────────────────────────────────────────
        VkClearValue gclr[5]{};
        gclr[0].color = gclr[1].color = gclr[2].color = gclr[3].color = {{0, 0, 0, 0}};
        gclr[4].depthStencil = {1, 0};
        VkRenderPassBeginInfo grp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        grp.renderPass = def_->geoRP();
        grp.framebuffer = def_->geoFB(ix % 3);
        grp.renderArea.extent = sw_->extent();
        grp.clearValueCount = 5;
        grp.pClearValues = gclr;
        vkCmdBeginRenderPass(cb, &grp, VK_SUBPASS_CONTENTS_INLINE);

        float aspect = (float) sw_->extent().width / (float) sw_->extent().height;
        glm::mat4 view = game_.viewMatrix();
        glm::mat4 proj = game_.projectionMatrix(aspect);

        uint32_t geomDrawCalls = 0;
        sceneRenderer_.drawGeometryPass(cb, scene_, meshCache_, *def_, sw_->extent(), view, proj, geomDrawCalls);
        vkCmdEndRenderPass(cb);

        // ── Lighting pass ────────────────────────────────────────────
        def_->beginLightingPass(cb, (uint32_t) (ix % 3), sw_->renderPass(), sw_->framebuffers()[ix], sw_->extent());
        sceneRenderer_.drawLightingPass(cb, scene_, *def_, game_.cameraPosition());
        editor_.setDrawCalls(geomDrawCalls + 1); // +1 for lighting
        editor_.endFrame(cb);
        vkCmdEndRenderPass(cb);
        vkEndCommandBuffer(cb);

        // ── Submit ───────────────────────────────────────────────────
        VkSemaphore ws[] = {sa_[fi_]};
        VkPipelineStageFlags wf[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = ws;
        si.pWaitDstStageMask = wf;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cb;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &sr_[fi_];
        vkQueueSubmit(ctx_->graphicsQueue(), 1, &si, fences_[fi_]);

        VkSwapchainKHR swc = sw_->swapchain();
        VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr, 1, &sr_[fi_], 1, &swc, &ix, nullptr};
        vkQueuePresentKHR(ctx_->graphicsQueue(), &pi);
        fi_ = (fi_ + 1) % MAX_FIF;
    }

    void resize()
    {
        int w = 0, h = 0;
        w_->getFramebufferSize(&w, &h);
        while (w == 0 || h == 0)
        {
            w_->getFramebufferSize(&w, &h);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(dev());
        w_->setSize(w, h);
        def_.reset();
        ctx_->recreateSwapchain(w, h);
        sw_ = &ctx_->swapchain();
        def_ = std::make_unique<Engine::Renderer::DeferredRenderer>();
        def_->init(dev(), ctx_->physicalDevice(), alloc_.get(), sm_.get(), w, h, sw_->renderPass());
        editor_.onSwapchainResize(sw_->renderPass());
    }

    void cleanup()
    {
        vkQueueWaitIdle(ctx_->graphicsQueue());
        vkDeviceWaitIdle(dev());
        int cw = 0, ch = 0;
        w_->getFramebufferSize(&cw, &ch);
        if (cw > 0 && ch > 0)
            SaveSettings(cw, ch);
        meshCache_.clear();
        for (int i = 0; i < MAX_FIF; i++)
        {
            if (fences_[i])
                vkWaitForFences(dev(), 1, &fences_[i], VK_TRUE, UINT64_MAX);
            vkDestroySemaphore(dev(), sr_[i], nullptr);
            vkDestroySemaphore(dev(), sa_[i], nullptr);
            vkDestroyFence(dev(), fences_[i], nullptr);
        }
        vkDestroyCommandPool(dev(), cp_, nullptr);
        cp_ = VK_NULL_HANDLE;
    }
};

int main()
{
    App app;
    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
