#include "image.hpp"
#include "utils.hpp"
#include "device.hpp"
#include "buffer.hpp"

namespace 
{

VkImageCreateInfo image_create_info(
    VkFormat const format,
    VkExtent2D const extent,
    VkImageUsageFlags const usage)
{
    VkImageCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = format;
    create_info.extent = {extent.width, extent.height, 1};
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;
    return create_info;
}


VkImageViewCreateInfo image_view_create_info(
    VkFormat const format,
    VkImage const image,
    VkImageAspectFlags const aspects) 
{
    VkImageViewCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.format = format;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.subresourceRange = {
        .aspectMask = aspects,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    return info;
}


} // namespace


Image::Image(
    Device& device,
    VkFormat const format,
    VkExtent2D const extent,
    VkImageUsageFlags const usage,
    VkImageAspectFlags const aspect):
    device_{device},
    format_{format},
    extent_{extent},
    usage_{usage},
    aspect_{aspect},
    image_{create_image()},
    memory_{allocate_memory()},
    view_{create_image_view()}
{}
 
VkImage Image::create_image() const
{
    auto const image_info = image_create_info(format_, extent_, usage_);
    VkImage image;
    utils::check_vk(vkCreateImage(device_.logical(), &image_info, nullptr, &image));
    return image;
}

VkImageView Image::create_image_view() const 
{
    auto const view_info = image_view_create_info(format_, image_, aspect_);
    VkImageView view;
    utils::check_vk(vkCreateImageView(device_.logical(), &view_info, nullptr, &view));
    return view;
}


VkDeviceMemory Image::allocate_memory() const 
{
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device_.logical(), image_, &mem_reqs);
    auto const memory = device_.allocate(mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    utils::check_vk(vkBindImageMemory(device_.logical(), image_, memory, 0));
    return memory;
}

Image::~Image() = default;


void Image::fill(GpuBuffer const& buffer)
{
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device_.logical(), image_, &mem_reqs);

    GpuBuffer staging_buffer 
    {
        device_,
        mem_reqs.size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    
    staging_buffer.copy_from(buffer);
    device_.immediate_submit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange subresources_range 
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        VkImageMemoryBarrier imageBarrier_toTransfer {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = image_;
        imageBarrier_toTransfer.subresourceRange = subresources_range;
        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imageBarrier_toTransfer
        );


        VkBufferImageCopy copyRegion {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = {extent_.width, extent_.height, 1};


        vkCmdCopyBufferToImage(cmd, staging_buffer.handle(), image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
    });
}

