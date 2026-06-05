#include "Engine/Platform/input.hpp"
#include <GLFW/glfw3.h>

namespace Engine
{
namespace Platform
{
static Input* getPtr(GLFWwindow* w)
{
    return reinterpret_cast<Input*>(glfwGetWindowUserPointer(w));
}

void Input::attach(GLFWwindow* w)
{
    glfwSetWindowUserPointer(w, reinterpret_cast<void*>(this));
    glfwSetKeyCallback(w, keyCb);
    glfwSetMouseButtonCallback(w, mouseBtnCb);
    glfwSetCursorPosCallback(w, cursorCb);
}

void Input::beginFrame()
{
    prevKeys_ = rawKeys_;
    prevMouseButtons_ = rawMouseButtons_;
    prevMousePos_ = mousePos_;
    mouseDelta_ = mousePos_ - prevMousePos_;
}

void Input::keyCb(GLFWwindow* w, int key, int, int action, int)
{
    auto* s = getPtr(w);
    if (!s)
        return;
    if (action == GLFW_PRESS)
        s->rawKeys_[key] = true;
    else if (action == GLFW_RELEASE)
        s->rawKeys_[key] = false;
}

void Input::mouseBtnCb(GLFWwindow* w, int btn, int action, int)
{
    auto* s = getPtr(w);
    if (!s)
        return;
    if (action == GLFW_PRESS)
        s->rawMouseButtons_[btn] = true;
    else if (action == GLFW_RELEASE)
        s->rawMouseButtons_[btn] = false;
}

void Input::cursorCb(GLFWwindow* w, double x, double y)
{
    auto* s = getPtr(w);
    if (!s)
        return;
    s->mousePos_ = {x, y};
}

bool Input::edge(const std::unordered_map<int, bool>& prev, const std::unordered_map<int, bool>& cur, int code, bool desiredCur) const
{
    auto ci = cur.find(code);
    auto pi = prev.find(code);
    bool cv = (ci != cur.end()) && ci->second;
    bool pv = (pi != prev.end()) && pi->second;
    return (cv == desiredCur) && (cv != pv);
}

bool Input::keyHeld(int k) const
{
    auto it = rawKeys_.find(k);
    return it != rawKeys_.end() && it->second;
}

bool Input::keyPressed(int k) const
{
    return edge(prevKeys_, rawKeys_, k, true);
}

bool Input::keyReleased(int k) const
{
    return edge(prevKeys_, rawKeys_, k, false);
}

bool Input::mouseButtonHeld(int b) const
{
    auto it = rawMouseButtons_.find(b);
    return it != rawMouseButtons_.end() && it->second;
}

bool Input::mouseButtonPressed(int b) const
{
    return edge(prevMouseButtons_, rawMouseButtons_, b, true);
}

bool Input::mouseButtonReleased(int b) const
{
    return edge(prevMouseButtons_, rawMouseButtons_, b, false);
}
} // namespace Platform
} // namespace Engine
