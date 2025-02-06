#include <unordered_map>
#include "logging.hpp"
#include "window.hpp"
#include "utility.hpp"

namespace {

const std::unordered_map<int, Action> mapping {
    {GLFW_KEY_W, Action::Forward},
    {GLFW_KEY_S, Action::Backward},
    {GLFW_KEY_A, Action::Left}, 
    {GLFW_KEY_D, Action::Right},
    {GLFW_KEY_LEFT_SHIFT, Action::Down},
    {GLFW_KEY_SPACE, Action::Up},
};

}

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
    glfwSetKeyCallback(win, [](GLFWwindow* window, int key, [[maybe_unused]] int scancode, int key_action, [[maybe_unused]] int mods) {
        auto const mapping_it = mapping.find(key);
        if (mapping_it == mapping.end()) return;
        auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        auto const action_idx = to_underlying(mapping_it->second);

        if (key_action == GLFW_RELEASE) 
        {
            self->actions_.reset(action_idx);
        }

        if (key_action == GLFW_PRESS) 
        {
            self->actions_.set(action_idx);
        }

    });
    return win;
}

std::vector<Action> Window::collect_actions() const {
    glfwPollEvents();
    std::vector<Action> player_actions;
    auto const max = to_underlying(Action::MAX_COUNT);
    for (int idx{}; idx < max; ++idx) 
    {
        if (actions_.test(idx))
        {
            player_actions.emplace_back(Action{idx});
        }
    }
    return player_actions;
}
