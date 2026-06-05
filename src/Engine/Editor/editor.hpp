#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
struct GLFWwindow;

namespace Engine
{
namespace Editor
{
struct ConsoleEntry
{
    enum class Level
    {
        Info,
        Warning,
        Error
    } level = Level::Info;
    std::string text;
};

class EditorUI
{
public:
    EditorUI();
    ~EditorUI();
    EditorUI(const EditorUI&) = delete;
    EditorUI& operator=(const EditorUI&) = delete;
    void init(GLFWwindow* w, VkInstance inst, VkPhysicalDevice pd, VkDevice d, uint32_t qf, VkQueue q, VkRenderPass rp, uint32_t ic);
    void shutdown();
    void beginFrame();
    void endFrame(VkCommandBuffer cb);

    void setFrameTime(float ms) { frameTime_ = ms; }

    void setDrawCalls(uint32_t n) { drawCalls_ = n; }

    void setCameraPos(float x, float y, float z)
    {
        camX_ = x;
        camY_ = y;
        camZ_ = z;
    }

    void setCameraActive(bool v) { cameraActive_ = v; }

    bool cameraActive() const { return cameraActive_; }

    void log(ConsoleEntry::Level lv, const char* fmt, ...);

    bool wantsQuit() const { return wantsQuit_; }

    bool pollResizeRequest(int& w, int& h) const
    {
        if (reqW_ > 0 && reqH_ > 0)
        {
            w = reqW_;
            h = reqH_;
            return true;
        }
        return false;
    }

    void clearResizeRequest()
    {
        reqW_ = 0;
        reqH_ = 0;
    }

    void setFramebufferSize(int w, int h)
    {
        curW_ = w;
        curH_ = h;
    }

    void onSwapchainResize(VkRenderPass newRP);

private:
    bool showHierarchy_ = true, showInspector_ = true, showViewport_ = true, showConsole_ = true, wantsQuit_ = false,
         cameraActive_ = false, shutdown_ = false;
    float frameTime_ = 0;
    uint32_t drawCalls_ = 0;
    float camX_ = 0, camY_ = 0, camZ_ = 0;
    std::vector<ConsoleEntry> console_;
    float inspectorRotY_ = 0, inspectorMetallic_ = 0, inspectorRoughness_ = 0.4f;
    glm::vec3 inspectorAlbedo_{0.8f, 0.3f, 0.1f};
    float fontScale_ = 1.0f;
    int curW_ = 0, curH_ = 0;
    int reqW_ = 0, reqH_ = 0;
    void drawMenuBar(), drawHierarchy(), drawInspector(), drawViewport(), drawConsole();
    static constexpr float kFontScaleOptions[] = {1.00f, 1.25f, 1.50f, 1.75f, 2.00f, 2.25f, 2.50f, 3.00f};

    static int fontScaleToIndex(float s)
    {
        for (int i = 0; i < 8; ++i)
            if (s <= kFontScaleOptions[i] + 0.01f)
                return i;
        return 7;
    }
};
} // namespace Editor
} // namespace Engine
