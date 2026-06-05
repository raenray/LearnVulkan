#include "Engine/Editor/editor.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdio>
#include <cstdarg>
#include <fstream>

namespace Engine
{
namespace Editor
{
static void checkVk(VkResult err)
{
    if (err)
        fprintf(stderr, "[ImGui] VkResult %d\n", err);
}

EditorUI::EditorUI() = default;

EditorUI::~EditorUI()
{
    shutdown();
}

void EditorUI::init(GLFWwindow* w, VkInstance inst, VkPhysicalDevice pd, VkDevice d, uint32_t qf, VkQueue q, VkRenderPass rp, uint32_t ic)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // io.IniFilename = nullptr;
    ImGui::StyleColorsDark();

    // ── DPI-aware font size ────────────────────────────────────────
    float xs = 1, ys = 1;
    glfwGetWindowContentScale(w, &xs, &ys);
    float dpi = (xs + ys) / 2.0f;
    if (dpi < 1.0f)
        dpi = 1.0f;
    if (dpi > 3.0f)
        dpi = 3.0f;
    float fontSize = 16.0f * dpi;
    fontScale_ = 1.0f; // combo starts at 100%; size already accounts for DPI

    // ── Load custom font with Chinese glyph support ────────────────
    const char* fontPath = ASSETS_DIR "/fonts/MapleMono-NF-CN-unhinted/MapleMono-NF-CN-Light.ttf";
    std::ifstream test(fontPath, std::ios::binary);
    if (test.good())
    {
        test.close();
        io.Fonts->AddFontFromFileTTF(fontPath, fontSize, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        io.FontGlobalScale = 1.0f;
    }
    else
    {
        // Fallback: default ImGui font
        io.Fonts->AddFontDefault();
        io.FontGlobalScale = dpi;
        fontScale_ = dpi;
        fprintf(stderr, "[Editor] Font not found: %s — using default\n", fontPath);
    }

    ImGui_ImplGlfw_InitForVulkan(w, true);
    ImGui_ImplVulkan_InitInfo ii{};
    ii.ApiVersion = VK_API_VERSION_1_0;
    ii.Instance = inst;
    ii.PhysicalDevice = pd;
    ii.Device = d;
    ii.QueueFamily = qf;
    ii.Queue = q;
    ii.DescriptorPoolSize = 100;
    ii.MinImageCount = ic;
    ii.ImageCount = ic;
    ii.PipelineCache = VK_NULL_HANDLE;
    ii.Allocator = nullptr;
    ii.CheckVkResultFn = checkVk;
    ii.PipelineInfoMain.RenderPass = rp;
    ii.PipelineInfoMain.Subpass = 0;
    ii.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ii.UseDynamicRendering = false;
    ImGui_ImplVulkan_Init(&ii);
    VkCommandPoolCreateInfo cpci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = qf;
    VkCommandPool cp;
    vkCreateCommandPool(d, &cpci, nullptr, &cp);
    VkCommandBufferAllocateInfo cbai{};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = cp;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cb;
    vkAllocateCommandBuffers(d, &cbai, &cb);
    VkCommandBufferBeginInfo cbbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &cbbi);
    ImGui_ImplVulkan_CreateMainPipeline(&ii.PipelineInfoMain);
    vkEndCommandBuffer(cb);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    vkQueueSubmit(q, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(q);
    vkDestroyCommandPool(d, cp, nullptr);
}

void EditorUI::shutdown()
{
    if (shutdown_)
        return;
    shutdown_ = true;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorUI::onSwapchainResize(VkRenderPass rp)
{
    ImGui_ImplVulkan_PipelineInfo pi{};
    pi.RenderPass = rp;
    pi.Subpass = 0;
    pi.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_CreateMainPipeline(&pi);
}

void EditorUI::beginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    drawMenuBar();
    if (showHierarchy_)
        drawHierarchy();
    if (showInspector_)
        drawInspector();
    if (showViewport_)
        drawViewport();
    if (showConsole_)
        drawConsole();
}

void EditorUI::endFrame(VkCommandBuffer cb)
{
    ImGui::Render();
    ImDrawData* dd = ImGui::GetDrawData();
    if (dd && dd->CmdListsCount > 0)
        ImGui_ImplVulkan_RenderDrawData(dd, cb);
}

void EditorUI::drawMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "ESC"))
                wantsQuit_ = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy_);
            ImGui::MenuItem("Inspector", nullptr, &showInspector_);
            ImGui::MenuItem("Viewport", nullptr, &showViewport_);
            ImGui::MenuItem("Console", nullptr, &showConsole_);
            ImGui::Separator();
            static const char* fontLabels[] = {"100%", "125%", "150%", "175%", "200%", "225%", "250%", "300%"};
            int idx = fontScaleToIndex(fontScale_);
            if (ImGui::Combo("Font Scale", &idx, fontLabels, 8))
            {
                fontScale_ = kFontScaleOptions[idx];
                ImGui::GetIO().FontGlobalScale = fontScale_;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EditorUI::drawHierarchy()
{
    ImGui::Begin("Hierarchy", &showHierarchy_);
    ImGui::TextColored({0.5f, 0.5f, 0.5f, 1}, "Scene");
    ImGui::Indent();
    ImGui::Selectable("Camera");
    ImGui::Selectable("Light");
    ImGui::Selectable("Sphere");
    ImGui::Unindent();
    ImGui::End();
}

void EditorUI::drawInspector()
{
    ImGui::Begin("Inspector", &showInspector_);
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("Rotation Y", &inspectorRotY_, 1.0f, -180, 180);
    }
    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Text("Pos: %.2f, %.2f, %.2f", camX_, camY_, camZ_);
        ImGui::Text("Frame: %.1f ms", frameTime_);
        ImGui::Text("Draws: %u", drawCalls_);
    }
    if (ImGui::CollapsingHeader("Material"))
    {
        ImGui::ColorEdit3("Albedo", &inspectorAlbedo_.x);
        ImGui::SliderFloat("Metallic", &inspectorMetallic_, 0, 1);
        ImGui::SliderFloat("Roughness", &inspectorRoughness_, 0.01f, 1);
    }
    ImGui::End();
}

void EditorUI::drawViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::Begin("Viewport", &showViewport_);
    if (cameraActive_)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.6f, 0.2f, 1});
        if (ImGui::Button("Camera ON (ESC to release)"))
            cameraActive_ = false;
        ImGui::PopStyleColor();
    }
    else
    {
        if (ImGui::Button("Start Free Camera"))
            cameraActive_ = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("W", &curW_, 0, 0, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    ImGui::TextUnformatted("x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("H", &curH_, 0, 0, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("80%"))
    {
        GLFWmonitor* m = glfwGetPrimaryMonitor();
        if (m)
        {
            const GLFWvidmode* v = glfwGetVideoMode(m);
            if (v)
            {
                reqW_ = (int) (v->width * 0.8f);
                reqH_ = (int) (v->height * 0.8f);
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("60%"))
    {
        GLFWmonitor* m = glfwGetPrimaryMonitor();
        if (m)
        {
            const GLFWvidmode* v = glfwGetVideoMode(m);
            if (v)
            {
                reqW_ = (int) (v->width * 0.6f);
                reqH_ = (int) (v->height * 0.6f);
            }
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70);
    ImGui::InputInt("##rw", &reqW_, 0, 0);
    ImGui::SameLine();
    ImGui::TextUnformatted("x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70);
    ImGui::InputInt("##rh", &reqH_, 0, 0);
    ImGui::SameLine();
    if (ImGui::Button("Apply") && reqW_ > 0 && reqH_ > 0)
    {
        // reqW_/reqH_ already set — polled by App
    }
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (sz.x > 0 && sz.y > 0)
    {
        float fps = frameTime_ > 0.0f ? 1000.0f / frameTime_ : 0.0f;
        ImVec4 fpsColor = fps >= 55.0f   ? ImVec4{0.2f, 1.0f, 0.3f, 1.0f}
                          : fps >= 25.0f ? ImVec4{1.0f, 1.0f, 0.0f, 1.0f}
                                         : ImVec4{1.0f, 0.2f, 0.2f, 1.0f};
        ImGui::Text("3D Viewport (%dx%d)  |  ", (int) sz.x, (int) sz.y);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(fpsColor, "%.0f FPS", fps);
        ImGui::SameLine(0, 0);
        ImGui::Text("  |  %.1f ms  |  %u draws", frameTime_, drawCalls_);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorUI::drawConsole()
{
    ImGui::Begin("Console", &showConsole_);
    if (ImGui::Button("Clear"))
        console_.clear();
    ImGui::SameLine();
    static bool as = true;
    ImGui::Checkbox("Auto-scroll", &as);
    ImGui::Separator();
    ImGui::BeginChild("scroll", {0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);
    for (auto& e : console_)
    {
        ImVec4 c{1, 1, 1, 1};
        const char* p = "";
        switch (e.level)
        {
            case ConsoleEntry::Level::Info:
                c = {1, 1, 1, 1};
                p = "[info] ";
                break;
            case ConsoleEntry::Level::Warning:
                c = {1, 1, 0, 1};
                p = "[warn] ";
                break;
            case ConsoleEntry::Level::Error:
                c = {1, 0.2f, 0.2f, 1};
                p = "[err] ";
                break;
        }
        ImGui::TextColored(c, "%s%s", p, e.text.c_str());
    }
    if (as)
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::End();
}

void EditorUI::log(ConsoleEntry::Level lv, const char* fmt, ...)
{
    char b[1024];
    va_list a;
    va_start(a, fmt);
    vsnprintf(b, sizeof(b), fmt, a);
    va_end(a);
    console_.push_back({lv, b});
    if (console_.size() > 500)
        console_.erase(console_.begin());
}
} // namespace Editor
} // namespace Engine
