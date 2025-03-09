#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>


struct Descriptions {
    struct Attribiute {
        VkVertexInputAttributeDescription position;
        VkVertexInputAttributeDescription color;
        VkVertexInputAttributeDescription uv;
    } attribute;
    VkVertexInputBindingDescription binding;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uv;
    
    static Descriptions descriptions();
    static constexpr uint8_t COUNT {3};
};

