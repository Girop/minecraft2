#pragma once
#include <vulkan/vulkan.h>

struct Viewport {
    Viewport() = default;
    explicit Viewport(VkExtent2D extent):
        viewport {
            .x = 0.f,
            .y = 0.f,
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .minDepth = 0.f,
            .maxDepth = 1.f
        },
        scissors{{0,0}, extent}
    {}

    VkViewport viewport;
    VkRect2D scissors;
};

