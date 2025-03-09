#pragma once
#include <functional>
#include "utils.hpp"
#include "commands.hpp"


class Device 
{
public:
    Device(VkInstance const instance, VkSurfaceKHR const surface);

    void immediate_submit(std::function<void(VkCommandBuffer cmd)> const& func);
    VkDeviceMemory allocate(VkMemoryRequirements const mem_reqs, VkMemoryPropertyFlags const props) const;
    void wait() const;

    GETTER(physical);
    GETTER(logical);
private:
    uint32_t find_memory_type_index(uint32_t const type_filter, VkMemoryPropertyFlags const props) const;

    VkPhysicalDevice physical_;
    VkDevice logical_;
    VkQueue queue_;
    CommandBuffer cmd_;
};

