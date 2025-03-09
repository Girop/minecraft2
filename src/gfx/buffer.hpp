#pragma once
#include <vulkan/vulkan.h>
#include "utils.hpp"

class Device;

// This gotta be the most C++'y class I have ever written
class GpuBuffer {
public:
    GpuBuffer(
        Device& device,
        VkDeviceSize const size,
        VkBufferUsageFlags const usage,
        VkMemoryPropertyFlags const props);

    GpuBuffer(GpuBuffer&& rhs);
    
    // Copy operator should always provide fully initalize object, which should refelect
    // values present in the original (rhs). In the case of buffers, there are two ways of copying
    // this class - with new allocation and without. Both are useful and needed, as such 
    // it is better to avoid mistakes by making two disctinct member functions instead of defining
    // operators / ctrs. 
    // If copy with allocation is used as copy ctr, I end up with acciedantly allocating and freeing
    // buffers in places you wouldn't expect and either way must define additional data-only-copy member 
    // function.
    // If copy without allocations is used, well, then copied objects end up in invalid state.
    GpuBuffer(GpuBuffer const&) = delete;
    GpuBuffer& operator=(GpuBuffer const&) = delete;
    GpuBuffer& operator=(GpuBuffer&&) = delete;

    ~GpuBuffer();

    GpuBuffer copy() const;
    void copy_from(GpuBuffer const& src);
    void fill(void const* data);

    CONST_GETTER(handle);
    CONST_GETTER(size);
private:
    VkDeviceMemory allocate_buffer() const;

    void destroy();

    Device& device_;
    VkDeviceSize size_;
    VkBufferUsageFlags usage_;
    VkMemoryPropertyFlags properties_;
    VkBuffer handle_;
    VkDeviceMemory memory_;
};

