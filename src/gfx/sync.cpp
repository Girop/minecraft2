#include "sync.hpp"
#include "device.hpp"

Fence::Fence(Device const& device, bool const signaled) :
    device_{device}
{
    VkFlags flags{};
    if (signaled) 
    {
        flags |= VK_FENCE_CREATE_SIGNALED_BIT;
    }

    VkFenceCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags
    };
    utils::check_vk(vkCreateFence(device.logical(), &info, nullptr, &handle_));

}

void Fence::wait_and_reset() 
{
    vkWaitForFences(device_.logical(), 1, &handle_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_.logical(), 1, &handle_);
}

Semaphore::Semaphore(Device const& device) 
{
    VkSemaphoreCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    utils::check_vk(vkCreateSemaphore(device.logical(), &info, nullptr, &handle_));
}
