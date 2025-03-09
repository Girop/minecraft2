#include "world.hpp"
#include <glm/gtc/constants.hpp>
#include "log.hpp"

namespace 
{

glm::vec3 calculate_movement(Action const action, float const speed, float const yaw)
{
    float delta {speed};
    glm::vec3 mov(0.0f);
    switch (action) {
        case Action::Backward:
            delta *= -1.f;
            [[fallthrough]];
        case Action::Forward:
            mov.z += sinf(yaw) * delta;
            mov.x += cosf(yaw) * delta;
            break;
        case Action::Left:
            delta *= -1.f;
            [[fallthrough]];
        case Action::Right:;
            mov.z += sinf(yaw + glm::half_pi<float>()) * delta;
            mov.x += cosf(yaw + glm::half_pi<float>()) * delta;
            break;
        case Action::Up:
            delta *= -1.f;
            [[fallthrough]];
        case Action::Down:
            mov.y += delta;
            break;
        default: break;
    }
    return mov;
}

constexpr glm::vec3 red{1.f, 0.f, 0.f};
constexpr glm::vec3 blue{0.f, 0.f, 1.f};
constexpr glm::vec3 green{0.f, 1.f, 0.f};

constexpr glm::vec2 left_lower {0.f, 0.f};
constexpr glm::vec2 left_upper {0.f, 1.f};
constexpr glm::vec2 right_lower {1.f, 0.f};
constexpr glm::vec2 right_upper {1.f, 1.f};

constexpr float p {0.5f};
const std::vector verticies {
    Vertex{{-p, -p, -p}, red, left_lower},  // 0
    Vertex{{-p, p, -p}, green, right_upper}, // 1
    Vertex{{p, p, -p}, blue, left_upper},   // 2
    Vertex{{p, -p, -p}, red, right_lower},   // 3
    Vertex{{p, -p, p}, green, left_lower},  // 4
    Vertex{{p, p, p}, blue, left_upper},    // 5
    Vertex{{-p, p, p}, red, right_lower},    // 6
    Vertex{{-p, -p, p}, green, right_upper}, // 7
};

const std::vector<uint16_t> indices
{
    0, 2, 1, 0, 3, 2, // front
    3, 2, 4, 5, 4, 2, // right
    1, 0, 6, 0, 7, 6, // left
    1, 2, 6, 2, 6, 5, // up
    0, 3, 7, 3, 7, 4, // down
    7, 4, 5, 7, 5, 6, // back
};

} // namespace

World::World(glm::uvec2 const& extent, float const time_per_tick) :
    camera_{extent},
    time_per_tick_{time_per_tick}
{
    debug("World initalized");
}


void World::tick(UserInput const& input) 
{
    camera_.update(player_position_, input.mouse_delta);
    for (auto action : input.user_actions) 
    {
        player_position_ += calculate_movement(action, 0.003, camera_.yaw);
    }
}

RenderData World::to_render() const
{
    RenderData data
    {
        .camera = camera_,
        .player_pos = player_position_,
        .vertices = verticies,
        .indices = indices,
    };
    return data;
}

