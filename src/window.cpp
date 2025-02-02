#include <fmt/format.h>
#include <unordered_map>
#include <fmt/format.h>
#include "window.hpp"

static const std::unordered_map<int, Action> mapping {
    {GLFW_KEY_W, Action::Forward},
    {GLFW_KEY_S, Action::Backward},
    {GLFW_KEY_A, Action::Left},
    {GLFW_KEY_D, Action::Right},
    {GLFW_KEY_LEFT_SHIFT, Action::Down},
    {GLFW_KEY_SPACE, Action::Up},
};


GLFWwindow* Window::create_handle(char const* name, int width, int height) const
{
    if (not glfwInit()) 
    {
        fmt::println("Failed to create window");
        std::abort();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto win = glfwCreateWindow(width, height, name, nullptr, nullptr);
    glfwSetInputMode(win, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetWindowUserPointer(win, (void*)this);
    glfwSetKeyCallback(win, [](GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) {
        if (action == GLFW_RELEASE) return;
        if (mapping.find(key) == mapping.end()) return;
        auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        self->actions_.emplace_back(mapping.at(key));
    });
    return win;
}

std::vector<Action> Window::collect_actions() {
    glfwPollEvents();
    auto&& collected {std::move(actions_)};
    actions_.clear();
    return collected;
}
