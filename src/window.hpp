#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <bitset>

enum class Action {
    Left,
    Right,
    Forward,
    Backward,
    Up,
    Down,
    MAX_COUNT
};

class Window {
public:
    Window(const char* name, int width = 800, int heighth = 600): 
        handle_{create_handle(name, width, heighth)} 
    {}
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

    std::vector<Action> collect_actions() const;
    
private:
    GLFWwindow* create_handle(char const* name, int width, int height) const;

    std::bitset<static_cast<size_t>(Action::MAX_COUNT)> actions_;
    GLFWwindow* handle_;
};
