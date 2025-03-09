#include "queues.hpp"
#include <vector>
#include <algorithm>
#include "log.hpp"

QueueFamily::QueueFamily(VkPhysicalDevice const device, VkSurfaceKHR const surface)
{
    uint32_t family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families;
    families.resize(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

    size_t idx {};
    auto const valid_queue = std::find_if(families.begin(), families.end(), [&](auto const& queue) {
        VkBool32 supports_present {false};
        vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, surface, &supports_present);
        ++idx;
        return (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) and supports_present;
    });

    if (valid_queue == families.end()) 
    {
        id_ = std::nullopt;
    } else {
        id_ = std::distance(families.begin(), valid_queue);
    }

}

VkQueue QueueFamily::get_queue(VkDevice const device) const 
{
    VkQueue queue;
    vkGetDeviceQueue(device, id_.value(), 0, &queue);
    return queue;
}

