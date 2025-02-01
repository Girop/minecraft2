#pragma once
#include <glm/glm.hpp>
#include <array>
#include <vulkan/vulkan.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

inline VkVertexInputBindingDescription binding_description() {
    return VkVertexInputBindingDescription {
        .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
}

inline std::array<VkVertexInputAttributeDescription, 2> attribute_descritptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributes{};
    auto& position {attributes.at(0)};
    position.binding = 0;
    position.location = 0;
    position.format = VK_FORMAT_R32G32B32_SFLOAT;
    position.offset = offsetof(Vertex, pos);

    auto& clr {attributes.at(1)};
    clr.binding = 0;
    clr.location = 1;
    clr.offset = offsetof(Vertex, color);
    clr.format = VK_FORMAT_R32G32B32_SFLOAT;

    return attributes;
}
