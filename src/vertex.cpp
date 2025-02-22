#include "vertex.hpp"


 VkVertexInputBindingDescription Vertex::binding_description() {
    return VkVertexInputBindingDescription {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
}

Vertex::AttributeDescriptions Vertex::attribute_descriptions() {
    AttributeDescriptions attributes;
    attributes.position = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, pos),
    };

    attributes.color = {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color),
    };

    attributes.uv = {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, uv)
    };

    return attributes;
}
