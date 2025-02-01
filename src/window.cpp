#include <fmt/format.h>
#include "window.hpp"

GLFWwindow* Window::create_handle(char const* name, int width, int height) const
{
    if (not glfwInit()) 
    {
        fmt::println("Failed to create window");
        std::abort();
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, name, nullptr, nullptr);
}
