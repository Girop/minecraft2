#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uv;

    static VkVertexInputBindingDescription binding_description();

    struct AttributeDescriptions {
        VkVertexInputAttributeDescription position;
        VkVertexInputAttributeDescription color;
        VkVertexInputAttributeDescription uv;
    };
    static AttributeDescriptions attribute_descriptions();
};


