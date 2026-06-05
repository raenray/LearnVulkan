#pragma once
#include <string>
#include <functional>
struct GLFWwindow;

namespace Engine
{
namespace Platform
{
class Window
{
public:
    Window(int w, int h, const char* title);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    bool shouldClose() const;
    void pollEvents();
    int width() const;
    int height() const;
    void getFramebufferSize(int* w, int* h) const;

    GLFWwindow* native() const { return window_; }

    /// Update the stored dimensions after a resize.
    void setSize(int w, int h)
    {
        width_ = w;
        height_ = h;
    }

    /// Request GLFW to resize the window (triggers framebuffer resize → swapchain recreation).
    void resizeWindow(int w, int h);

private:
    GLFWwindow* window_ = nullptr;
    int width_ = 800, height_ = 600;
};
} // namespace Platform
} // namespace Engine
