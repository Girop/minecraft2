#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <algorithm>
#include "window.hpp"


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;

    static SwapChainSupportDetails create(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
    
    VkExtent2D choose_swap_extent(Window const& window) const {
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

    VkSurfaceFormatKHR choose_format() const {
        for (auto const& format : formats) {
            if (format.format == VK_FORMAT_R8G8B8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return formats.front();
    }

    VkPresentModeKHR choose_present_mode() const {
        for (auto const& mode : present_modes) {
           if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
               return mode;
           }
        }
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    uint32_t image_count() const {
        uint32_t image_count {capabilities.minImageCount + 1};
        if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
            image_count = capabilities.maxImageCount;
        }
        return image_count;
        
    }

    bool supported() const {
        return !formats.empty() and !present_modes.empty();
    }
};

struct Swapchain {
    static Swapchain create(
        SwapChainSupportDetails const& details,
        VkPhysicalDevice phys_device,
        VkDevice device,
        VkSurfaceKHR surface,
        Window const& window
    );

    void recreate(Window const& window, VkPhysicalDevice phys_device, VkDevice device, VkSurfaceKHR surface);
    void destroy(VkDevice device);
    
    VkSwapchainKHR swapchain;
    VkFormat color_format;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
};

