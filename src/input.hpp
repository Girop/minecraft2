#pragma once
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include "utils.hpp"
#include "interfaces.hpp"


struct Key 
{
    enum State {
        Inactive,
        JustPressed,
        ContinuslyPressed,
        Released
    };

    Key() = default;
    Key(int code_in, State state_in): code{code_in}, state{state_in} {}

    auto operator<=>(const Key& other) const = default;

    int code{GLFW_KEY_UNKNOWN};
    State state{Inactive};
};

class Mouse 
{
public:
    Mouse(GLFWwindow* window): 
        window_{window} {}

    void grab_mouse();
    void release_mouse();
    bool is_grabbed() const 
    {
        return grabbed_;
    }

    GETTER(delta);
    GETTER(position);
private:
    GLFWwindow* window_;
    bool grabbed_ {false};
    glm::vec2 delta_{0.f, 0.f};
    glm::vec2 position_{0.f, 0.f};
};



class InputCollector 
{
public:
    InputCollector(GLFWwindow* window);
    void update();
    UserInput actions() const;
private:
    static void key_callback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int key_action, [[maybe_unused]] int mods);
    static void mouse_callback(GLFWwindow* window, double x_pos, double y_pos);

    std::map<Key, Action> action_mappings_;
    std::vector<Key> keys_;
    Mouse mouse_;
};

