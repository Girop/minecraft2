#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct QueueFamilyIndicies {

std::optional<uint32_t> graphics;
std::optional<uint32_t> present;

    static QueueFamilyIndicies from(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndicies indicies;

        uint32_t queue_famliy_count{};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_famliy_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families;
        queue_families.resize(queue_famliy_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_famliy_count, queue_families.data());

        int queue_idx{};
        for (auto const& queue: queue_families) {
            if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indicies.graphics = queue_idx;
            }

            VkBool32 supports_present {false};
            vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_idx, surface, &supports_present);
            if (supports_present) {
                indicies.present = queue_idx;
            }

            queue_idx++;
        }
        return indicies;
    }

    VkQueue graphics_queue(VkDevice device) {
        VkQueue queue;
        vkGetDeviceQueue(device, graphics.value(), 0, &queue);
        return queue;
    }

    VkQueue present_queue(VkDevice device) {
        VkQueue queue;
        vkGetDeviceQueue(device, present.value(), 0, &queue);
        return queue;
    }


    bool complete() const {
        return graphics.has_value() and present.has_value();
    }
};

