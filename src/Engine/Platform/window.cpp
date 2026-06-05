#include "Engine/Platform/window.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace Engine
{
namespace Platform
{
Window::Window(int w, int h, const char* title)
    : width_(w)
    , height_(h)
{
    if (!glfwInit())
        throw std::runtime_error("Window: glfwInit failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window_)
    {
        glfwTerminate();
        throw std::runtime_error("Window: glfwCreateWindow failed");
    }
}

Window::~Window()
{
    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(window_);
}

void Window::pollEvents()
{
    glfwPollEvents();
}

int Window::width() const
{
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    return w;
}

int Window::height() const
{
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    return h;
}

void Window::getFramebufferSize(int* w, int* h) const
{
    glfwGetFramebufferSize(window_, w, h);
}

void Window::resizeWindow(int w, int h)
{
    glfwSetWindowSize(window_, w, h);
    width_ = w;
    height_ = h;
}
} // namespace Platform
} // namespace Engine
