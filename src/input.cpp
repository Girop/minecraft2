#include "input.hpp"
#include "log.hpp"

void Mouse::grab_mouse() 
{
    grabbed_ = true;
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Mouse::release_mouse() 
{
    grabbed_ = false;
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

InputCollector::InputCollector(GLFWwindow* window):
    action_mappings_{
        {{GLFW_KEY_W, Key::ContinuslyPressed}, Action::Forward},
        {{GLFW_KEY_S, Key::ContinuslyPressed}, Action::Backward},
        {{GLFW_KEY_A, Key::ContinuslyPressed}, Action::Left},
        {{GLFW_KEY_D, Key::ContinuslyPressed}, Action::Right},
        {{GLFW_KEY_LEFT_SHIFT, Key::ContinuslyPressed}, Action::Down},
        {{GLFW_KEY_SPACE, Key::ContinuslyPressed}, Action::Up},
        {{GLFW_KEY_ESCAPE, Key::JustPressed}, Action::Terminate},
    },
    keys_{[]() -> std::vector<Key> {
        std::vector<Key> keys;
        keys.resize(GLFW_KEY_LAST);
        for (int i {}; i < GLFW_KEY_LAST; ++i) 
        {
            keys[i] = {i, Key::Inactive};
        }
        return keys;
    }()},
    mouse_{window}
{
    glfwSetWindowUserPointer(window, (void*)this);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    debug("InputCollector initalized");
}

void InputCollector::update() 
{
    for (auto& key : keys_) 
    {
        if (key.state == Key::JustPressed) 
        {
            key.state = Key::ContinuslyPressed;
        }
    }
}


void InputCollector::key_callback(
    GLFWwindow* window,
    int key,
    [[maybe_unused]] int scancode,
    int key_action,
    [[maybe_unused]] int mods) 
{
    auto self = static_cast<InputCollector*>(glfwGetWindowUserPointer(window));
    if (static_cast<size_t>(key) > self->keys_.size()) 
    {
        warn("Unkown key value: {}", key);
        return;
    }

    auto& own_key = self->keys_.at(key);
    switch (key_action) {
        case GLFW_RELEASE:
            own_key.state = Key::Inactive;
            break;
        case GLFW_PRESS:
            own_key.state = Key::JustPressed;
            break;
        default:
            break;
    }; 
}

void InputCollector::mouse_callback(GLFWwindow* window, double x_pos, double y_pos) {
    auto self = static_cast<InputCollector*>(glfwGetWindowUserPointer(window));
    glm::vec2 const new_pos {x_pos, y_pos};
    self->mouse_.delta() = glm::clamp(new_pos - self->mouse_.position(), -100.f, 100.0f);
    self->mouse_.position() = new_pos;
}

UserInput InputCollector::actions() const
{
    UserInput input;
    input.mouse_delta = mouse_.delta();
    input.mouse_position = mouse_.position();
    for (auto& key : keys_) 
    {
        auto action_it = action_mappings_.find(key);
        if (action_it == action_mappings_.end()) continue;
        input.user_actions.emplace_back(action_it->second);
    }
    return input;
}
