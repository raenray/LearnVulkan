#pragma once
#include <unordered_map>
#include <glm/glm.hpp>
struct GLFWwindow;
namespace Engine { namespace Platform {
class Input {
public:
    Input() = default;
    void attach(GLFWwindow* w);
    void beginFrame();
    bool keyHeld(int key) const;
    bool keyPressed(int key) const;
    bool keyReleased(int key) const;
    bool mouseButtonHeld(int b) const;
    bool mouseButtonPressed(int b) const;
    bool mouseButtonReleased(int b) const;
    glm::dvec2 mousePosition() const { return mousePos_; }
    glm::dvec2 mouseDelta() const { return mouseDelta_; }
private:
    std::unordered_map<int,bool> rawKeys_, rawMouseButtons_, prevKeys_, prevMouseButtons_;
    glm::dvec2 mousePos_{0,0}, prevMousePos_{0,0}, mouseDelta_{0,0};
    bool edge(const std::unordered_map<int,bool>& prev, const std::unordered_map<int,bool>& cur, int code, bool desiredCur) const;
    static void keyCb(GLFWwindow*,int,int,int,int);
    static void mouseBtnCb(GLFWwindow*,int,int,int);
    static void cursorCb(GLFWwindow*,double,double);
};
} }
