#include <span>
#include "queues.hpp"
#include "swapchain.hpp"
#include "utility.hpp"


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

Swapchain Swapchain::create(
    SwapChainSupportDetails const& details,
    VkPhysicalDevice phys_device,
    VkDevice device,
    VkSurfaceKHR surface,
    Window const& window
) {
    auto format = details.choose_format();
    auto extent = details.choose_swap_extent(window);
    auto indicies = QueueFamilyIndicies::from(phys_device, surface);
    std::array queue_fam_indices {indicies.graphics.value(), indicies.present.value()};
    auto create_info = swapchain_create_info(surface, format, details, extent, queue_fam_indices);
    VkSwapchainKHR swapchain;
    check_vk(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain));

    auto images = create_images(swapchain, device);
    auto views = create_views(device, images, format.format);

    return {
        swapchain, 
        format.format,
        images,
        views
    };
}

void Swapchain::recreate(Window const& window, VkPhysicalDevice phys_device, VkDevice device, VkSurfaceKHR surface)
{
    vkDeviceWaitIdle(device);
    destroy(device);
    auto size = window.size();
    while (size.x == 0 or size.y == 0) {
        size = window.size();
        glfwWaitEvents();
    }
    auto details = SwapChainSupportDetails::create(phys_device, surface);
    *this = Swapchain::create(details, phys_device, device, surface, window);
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
