#pragma once
#include <vulkan/vulkan.h>


struct Device {
    VkPhysicalDevice physical;
    VkDevice logical;

    static Device create(VkInstance instance, VkSurfaceKHR surface);
};

