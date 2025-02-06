#pragma once
#include <vulkan/vulkan.h>

struct Image {
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
};

