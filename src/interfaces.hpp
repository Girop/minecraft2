#pragma once
#include <vector>
#include "camera.hpp"
#include "gfx/vertex.hpp"

enum class Action {
    Left,
    Right,
    Forward,
    Backward,
    Up,
    Down,
    Terminate,
    MAX_COUNT
};

// User -> World
struct UserInput
{
    std::vector<Action> user_actions;
    glm::vec2 mouse_delta;
    glm::vec2 mouse_position;
};


// World -> renderer
struct RenderData 
{
    PerspectiveCamera camera;
    glm::vec3 player_pos;
    // This shouldn't be a dynamic allocated container
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

