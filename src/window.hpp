#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <bitset>
#include "utility.hpp"

enum class Action {
    Left,
    Right,
    Forward,
    Backward,
    Up,
    Down,
    Terminate,
    MAX_COUNT
};

class Window {

    struct Mouse;

public:
    Window(const char* name, int width = 800, int heighth = 600): 
        handle_{create_handle(name, width, heighth)}
    {
        glfwSetWindowUserPointer(handle_, (void*)this);
        grab_mouse();
    }
    Window(Window const&) = delete;
    Window operator=(Window const&) = delete;
    Window(Window&&) noexcept = default;
    Window& operator=(Window&&) noexcept = default;
    ~Window() {
        glfwDestroyWindow(handle_);
    }

    GLFWwindow* handle() const {
        return handle_;
    }

    glm::uvec2 size() const {
        int width, height;
        glfwGetFramebufferSize(handle_, &width, &height);
        return {width, height};
    }
    
    bool should_close() const {
        return glfwWindowShouldClose(handle_);
    }

    void grab_mouse();
    void release_mouse();
    bool is_grabbed() const {
        return mouse_.grabbed;
    }

    std::vector<Action> collect_actions() const;

    GETTER(Mouse, mouse);
private:
    static GLFWwindow* create_handle(char const* name, int width, int height);
    
    GLFWwindow* handle_;
    struct Mouse {
        bool grabbed {false};
        glm::vec2 movement{0.f, 0.f};
        glm::vec2 position {0.f, 0.f};
    } mouse_;
    std::bitset<to_underlying(Action::MAX_COUNT)> actions_;
};

