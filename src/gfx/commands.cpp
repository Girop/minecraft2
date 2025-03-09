#include "commands.hpp"
#include "queues.hpp"
#include "device.hpp"

namespace  
{

VkCommandPool allocate_command_pool(Device const& device, VkSurfaceKHR const surface) {

    QueueFamily queues {device.physical(), surface};
    VkCommandPoolCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queues.id()
    };
    VkCommandPool pool;
    utils::check_vk(vkCreateCommandPool(device.logical(), &info, nullptr, &pool));
    return pool;
}

VkCommandBuffer allocate_command_buffer(Device const& device, VkCommandPool const pool) 
{
    VkCommandBufferAllocateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer buffer;
    utils::check_vk(vkAllocateCommandBuffers(device.logical(), &info, &buffer));
    return buffer;
}

VkSubmitInfo submit_info(VkCommandBuffer const& buffer)
{
    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;
    return submit_info;
}

}

CommandBuffer::CommandBuffer(Device const& device, VkSurfaceKHR const surface, VkQueue const queue):
    pool_{allocate_command_pool(device, surface)},
    buffer_{allocate_command_buffer(device, pool_)},
    queue_{queue},
    execution_fence_{device, true}
{}


void CommandBuffer::record(std::function<void(VkCommandBuffer)> const& context, bool const single_use) 
{

    VkCommandBufferBeginInfo buffer_begin_info{};
    buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (single_use) 
    {
        buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    utils::check_vk(vkBeginCommandBuffer(buffer_, &buffer_begin_info));
    context(buffer_);
    utils::check_vk(vkEndCommandBuffer(buffer_));
}

void CommandBuffer::submit_default()
{
    auto const info = submit_info(buffer_);
    submit(info);
}

void CommandBuffer::submit(VkSubmitInfo const& submit_info)
{
    utils::check_vk(vkQueueSubmit(queue_, 1, &submit_info, execution_fence_.handle()));
}

void CommandBuffer::wait()
{
    execution_fence_.wait_and_reset();
}

void CommandBuffer::reset()
{
    vkResetCommandBuffer(buffer_, 0);
}

