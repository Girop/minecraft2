#pragma once
#include <vulkan/vulkan.h>
#include <functional>
#include "fence.hpp"

class Device;

class CommandBuffer {
public:
    CommandBuffer(Device const& device, VkSurfaceKHR const surface, VkQueue const queue, bool signaled = true);

    CommandBuffer(CommandBuffer const&) = delete;
    CommandBuffer operator=(CommandBuffer const&) = delete;

    void record(std::function<void(VkCommandBuffer)> const& context, bool const single_use = false);
    void submit_default();
    void submit(VkSubmitInfo const& submit_info);
    void wait();
    void reset();

    GETTER(execution_fence);
    GETTER(buffer);
private:
    VkCommandPool pool_;
    VkCommandBuffer buffer_;
    VkQueue queue_;
    Fence execution_fence_;
};

