#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Window {
public:
    Window(const char* name, int width = 800, int heighth = 600): 
        handle_{create_handle(name, width, heighth)} 
    {}

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

    Window(Window const&) = delete;
    Window operator=(Window const&) = delete;

    Window(Window&&) noexcept = default;
    Window& operator=(Window&&) noexcept = default;

    ~Window() {
        glfwDestroyWindow(handle_);
    }

private:
    GLFWwindow* create_handle(char const* name, int width, int height) const;

    GLFWwindow* handle_;
};
