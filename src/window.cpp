#include <unordered_map>
#include "logging.hpp"
#include "window.hpp"

// Nearly all directions are inverted, it is hack until I understand coordinate system
static const std::unordered_map<int, Action> mapping {
    {GLFW_KEY_W, Action::Backward},
    {GLFW_KEY_S, Action::Forward},
    {GLFW_KEY_A, Action::Left}, 
    {GLFW_KEY_D, Action::Right},
    {GLFW_KEY_LEFT_SHIFT, Action::Up},
    {GLFW_KEY_SPACE, Action::Down},
};


GLFWwindow* Window::create_handle(char const* name, int width, int height) const
{
    if (not glfwInit()) 
    {
        fail("Failed to create window");
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
    const auto collected {actions_};
    actions_.clear();
    return collected;
}
