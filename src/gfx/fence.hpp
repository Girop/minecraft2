#pragma once
#include "utils.hpp"

class Device;

class Fence {
public:
    Fence(Device const& device, bool const signaled);

    void wait_and_reset();
    bool is_signaled() const;

    GETTER(handle);
private:
    Device const& device_;
    VkFence handle_;
};

