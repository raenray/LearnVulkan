#pragma once
#include <string>
#include <functional>
struct GLFWwindow;
namespace Engine { namespace Platform {
class Window {
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
private:
    GLFWwindow* window_ = nullptr;
    int width_ = 800, height_ = 600;
};
} }
