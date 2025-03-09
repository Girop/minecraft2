#pragma once
#include <vulkan/vulkan.h>
#include "utils.hpp"

class Device;

class Fence {
public:
    Fence(Device const& device, bool const signaled);

    void wait_and_reset();
    GETTER(handle);
private:
    Device const& device_;
    VkFence handle_;
};


class Semaphore {
public:
    explicit Semaphore(Device const& device);
    GETTER(handle);
private:
    VkSemaphore handle_;
};
