#pragma once
#include "camera.hpp"
#include "interfaces.hpp"


class World 
{
public:
    World(glm::uvec2 const& extent, float const time_per_tick);
    void tick(UserInput const& input);
    RenderData to_render() const;
private:
    PerspectiveCamera camera_;
    float time_per_tick_;
    glm::vec3 player_position_{-2.f, .0f, .0f};
    uint32_t tick_number{0};

};
