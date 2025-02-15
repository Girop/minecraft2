#include <span>
#include <algorithm>
#include "device.hpp"
#include "queues.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include "utility.hpp"


SwapChainSupportDetails SwapChainSupportDetails::create(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count {};
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    } 

    uint32_t presentation_modes_count{};
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_modes_count, nullptr);
    if (presentation_modes_count != 0) {
        details.present_modes.resize(presentation_modes_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_modes_count, details.present_modes.data());
    }
    return details;
}


VkExtent2D SwapChainSupportDetails::choose_swap_extent(Window const& window) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    auto size = window.size();
    VkExtent2D extent {
        .width = static_cast<uint32_t>(size.x),
        .height = static_cast<uint32_t>(size.y)
    };
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return extent;
}

VkSurfaceFormatKHR SwapChainSupportDetails::choose_format() const
{
    for (auto const& format : formats) {
        if (format.format == VK_FORMAT_R8G8B8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats.front();
}

VkPresentModeKHR SwapChainSupportDetails::choose_present_mode() const
{
    for (auto const& mode : present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

uint32_t SwapChainSupportDetails::image_count() const
{
    uint32_t image_count {capabilities.minImageCount + 1};
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }
    return image_count;
}



VkSwapchainCreateInfoKHR swapchain_create_info(
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR format,
    SwapChainSupportDetails const& details,
    VkExtent2D extent,
    std::span<uint32_t> queue_indices
) {
    VkSwapchainCreateInfoKHR create_info {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
        .minImageCount = details.image_count(),
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = static_cast<uint32_t>(queue_indices.size()),
        .pQueueFamilyIndices = queue_indices.data(),
        .preTransform = details.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = details.choose_present_mode(),
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };
    return create_info;
}

std::vector<VkImage> create_images(VkSwapchainKHR swapchain, VkDevice device) {
    uint32_t image_count{};
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    std::vector<VkImage> images;
    images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());
    return images;
}

std::vector<VkImageView> create_views(
    VkDevice device,
    std::span<VkImage> images,
    VkFormat color_format
) {
    size_t image_count {images.size()};
    std::vector<VkImageView> views;
    views.resize(image_count);

    for (size_t idx{}; idx < image_count; ++idx) {
        VkImageViewCreateInfo info {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = images[idx],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = color_format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        check_vk(vkCreateImageView(device, &info, nullptr, &views[idx]));
    }
    return views;
}

Swapchain Swapchain::create(Device const& device, VkSurfaceKHR surface, Window const& window) {
    auto details = SwapChainSupportDetails::create(device.physical, surface);
    auto format = details.choose_format();
    auto extent = details.choose_swap_extent(window);
    auto indicies = QueueFamilyIndicies::from(device.physical, surface);
    std::array queue_fam_indices {indicies.graphics.value(), indicies.present.value()};
    auto create_info = swapchain_create_info(surface, format, details, extent, queue_fam_indices);
    VkSwapchainKHR swapchain;
    check_vk(vkCreateSwapchainKHR(device.logical, &create_info, nullptr, &swapchain));

    auto images = create_images(swapchain, device.logical);
    auto views = create_views(device.logical, images, format.format);

    return {
        details,
        swapchain, 
        format.format,
        images,
        views
    };
}

void Swapchain::recreate(Device const& device, VkSurfaceKHR surface, Window const& window)
{
    vkDeviceWaitIdle(device.logical);
    destroy(device.logical);
    auto size = window.size();
    while (size.x == 0 or size.y == 0) {
        size = window.size();
        glfwWaitEvents();
    }
    *this = Swapchain::create(device, surface, window);
}

void Swapchain::destroy(VkDevice device) 
{
    for (auto image_view : views) {
        vkDestroyImageView(device, image_view, nullptr);
    }

    for (auto image : images) {
        vkDestroyImage(device, image, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

