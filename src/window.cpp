#include "window.hpp"
#include "log.hpp"


Window::Window(const char* name, int width, int heighth):
    handle_{create_handle(name, width, heighth)}
{
    debug("Window initalized");
}

GLFWwindow* Window::create_handle(char const* name, int width, int height)
{
    if (not glfwInit()) 
    {
        fail("Failed to create window");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto win = glfwCreateWindow(width, height, name, nullptr, nullptr); 
    glfwSetInputMode(win, GLFW_STICKY_KEYS, GLFW_TRUE);
    return win;
}


Window::~Window() 
{
    glfwDestroyWindow(handle_);
}

glm::uvec2 Window::size() const
{
    int width, height;
    glfwGetFramebufferSize(handle_, &width, &height);
    return {width, height};
}


bool Window::should_close() const
{
    return glfwWindowShouldClose(handle_);
}

void Window::poll() const 
{
    glfwPollEvents();
}
