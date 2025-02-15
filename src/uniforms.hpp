#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "buffer.hpp"


struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

};

struct UniformBuffer {
    Buffer buffer;
    void* mapped;

    void copy(UniformBufferObject const& ubo) {
        memcpy(mapped, &ubo, sizeof ubo);
    }
};

