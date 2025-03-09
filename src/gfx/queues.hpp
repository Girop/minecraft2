#pragma once
#include <vulkan/vulkan.h>
#include <optional>

class QueueFamily 
{
public:
    // Device cannot be used here, as this class is created before its creation
    QueueFamily(VkPhysicalDevice const device, VkSurfaceKHR const surface);
    
    bool exists() const 
    {
        return id_.has_value();
    }

    uint32_t id() const
    {
        return id_.value();
    }

    VkQueue get_queue(VkDevice const device) const;
    
private:
    std::optional<uint32_t> id_;
};

