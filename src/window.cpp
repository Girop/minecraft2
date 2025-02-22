#include <unordered_map>
#include "window.hpp"
#include "utils/log.hpp"
#include "utils/enums.hpp"

namespace {

const std::unordered_map<int, Action> mapping {
    {GLFW_KEY_W, Action::Forward},
    {GLFW_KEY_S, Action::Backward},
    {GLFW_KEY_A, Action::Left}, 
    {GLFW_KEY_D, Action::Right},
    {GLFW_KEY_LEFT_SHIFT, Action::Down},
    {GLFW_KEY_SPACE, Action::Up},
    {GLFW_KEY_ESCAPE, Action::Terminate},
    {GLFW_KEY_L, Action::Log},
};

}

Window::Window(const char* name, int width, int heighth):
    handle_{create_handle(name, width, heighth)}
{
    glfwSetWindowUserPointer(handle_, (void*)this);
    grab_mouse();
}

GLFWwindow* Window::create_handle(char const* name, int width, int height)
{
    if (not glfwInit()) 
    {
        fail("Failed to create window");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto win = glfwCreateWindow(width, height, name, nullptr, nullptr); 
    glfwSetInputMode(win, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetKeyCallback(win, [](GLFWwindow* window, int key, [[maybe_unused]] int scancode, int key_action, [[maybe_unused]] int mods) {
        auto const mapping_it = mapping.find(key);
        if (mapping_it == mapping.end()) return;
        auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        auto const action_idx = utils::to_underlying(mapping_it->second);

        if (key_action == GLFW_RELEASE) 
        {
            self->actions_.reset(action_idx);
        }

        if (key_action == GLFW_PRESS) 
        {
            self->actions_.set(action_idx);
        }

    });
    glfwSetCursorPosCallback(win, [](GLFWwindow* window, double x_pos, double y_pos) {
        auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        glm::vec2 const new_pos {x_pos, y_pos};
        self->mouse_.movement = glm::clamp(new_pos - self->mouse_.position, -100.f, 100.0f);
        self->mouse_.position = new_pos;
    });
    return win;
}

void Window::grab_mouse() {
    mouse_.grabbed = true;
    glfwSetInputMode(handle_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::release_mouse() {
    mouse_.grabbed = false;
    glfwSetInputMode(handle_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

std::vector<Action> Window::collect_actions() const {
    glfwPollEvents();
    std::vector<Action> player_actions;
    for (int idx{}; idx < utils::to_underlying(Action::MAX_COUNT); ++idx) 
    {
        if (actions_.test(idx))
        {
            player_actions.emplace_back(Action{idx});
        }
    }
    return player_actions;
}
