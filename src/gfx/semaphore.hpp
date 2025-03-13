#pragma once
#include "utils.hpp"

class Device;

class Semaphore {
public:
    explicit Semaphore(Device const& device);
    GETTER(handle);
private:
    VkSemaphore handle_;
};
