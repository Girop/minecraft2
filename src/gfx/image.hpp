#pragma once
#include <vulkan/vulkan.h>
#include "utils.hpp"

class Device;
class GpuBuffer;

class Image 
{
public:
    Image(
        Device& device,
        VkFormat const format,
        VkExtent2D const extent,
        VkImageUsageFlags const usage,
        VkImageAspectFlags const aspect
    );

    void fill(GpuBuffer const& buffer);

    ~Image();

    CONST_GETTER(view);
private:
    VkImage create_image() const;
    VkDeviceMemory allocate_memory() const;
    VkImageView create_image_view() const;

    Device& device_;
    VkFormat format_;
    VkExtent2D extent_;
    VkImageUsageFlags usage_;
    VkImageAspectFlags aspect_;

    VkImage image_;
    VkDeviceMemory memory_;
    VkImageView view_;
};

