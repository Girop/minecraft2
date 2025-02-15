#pragma once
#include <glm/glm.hpp>
#include "utils/getter.hpp"
#include "interfaces.hpp"


class Player : IUpdatable{
public:
    void update() override;
    
    GETTER(glm::vec3, position)
private:
    glm::vec3 position_;
};
