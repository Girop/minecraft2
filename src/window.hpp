#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "utils.hpp"


class Window {
public:
    Window(const char* name, int width, int height);

    Window(Window const&) = delete;
    Window(Window&&) = default;
    Window& operator=(Window const&) = delete;
    Window& operator=(Window&&)  = default;

    ~Window();

    glm::uvec2 size() const; 
    bool should_close() const;
    void poll() const;

   GETTER(handle) ;
private:
    static GLFWwindow* create_handle(char const* name, int width, int height);
    
    GLFWwindow* handle_;
};

