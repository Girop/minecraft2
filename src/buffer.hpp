#pragma once
#include <vulkan/vulkan.h>

struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    size_t size;
};

