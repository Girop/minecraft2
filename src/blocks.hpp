#pragma once
#include <array>
#include <glm/glm.hpp>


struct Block {
    glm::vec3 position;
};


struct Chunk {
    std::array<std::array<Block, 256>, 256> blocks;
};

