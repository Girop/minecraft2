#include <assert.h>
#include "buffer.hpp"
#include "device.hpp"


VkDeviceMemory GpuBuffer::allocate_buffer() const 
{
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device_.logical(), handle_, &mem_reqs);
    auto const device_memory = device_.allocate(mem_reqs, properties_);
    vkBindBufferMemory(device_.logical(), handle_, device_memory, 0);
    return device_memory;
}

GpuBuffer::GpuBuffer(
    Device& device,
    VkDeviceSize const size,
    VkBufferUsageFlags const usage,
    VkMemoryPropertyFlags const props):
    device_{device},
    size_{size},
    usage_{usage},
    properties_{props}
{
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    utils::check_vk(vkCreateBuffer(device_.logical(), &create_info, nullptr, &handle_));
    memory_ = allocate_buffer();
}

GpuBuffer::GpuBuffer(GpuBuffer&& rhs): 
    device_{rhs.device_},
    size_{rhs.size_},
    usage_{rhs.usage_},
    properties_{rhs.properties_},
    handle_{rhs.handle_},
    memory_{rhs.memory_}
{
    rhs.memory_ = 0;
}

GpuBuffer::~GpuBuffer() 
{
    if (memory_ != 0) destroy();
}


void GpuBuffer::destroy() 
{
    vkDestroyBuffer(device_.logical(), handle_, nullptr);
    vkFreeMemory(device_.logical(), memory_, nullptr);
    size_ = 0;
    usage_ = 0;
    properties_ = 0;
}

// TODO Make size more explicit
void GpuBuffer::fill(void const* data) 
{
    void *maped;
    vkMapMemory(device_.logical(), memory_, 0, size_, 0, &maped);
    memcpy(maped, data, size_);
    vkUnmapMemory(device_.logical(), memory_);
}

void GpuBuffer::copy_from(GpuBuffer const& rhs)
{
    assert(size_ >= rhs.size_);
    device_.immediate_submit([&](VkCommandBuffer cmd){
        VkBufferCopy copy_info
        {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = rhs.size_,
        };
        vkCmdCopyBuffer(cmd, rhs.handle_, handle_, 1, &copy_info);
    });
}


GpuBuffer GpuBuffer::copy() const 
{
    GpuBuffer copied {device_, size_, usage_, properties_};
    copied.copy_from(*this);
    return copied;
}

