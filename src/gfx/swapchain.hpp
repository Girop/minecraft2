#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <optional>
#include "sync.hpp"

class Device;
class Window;

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;

    static SwapChainSupportDetails create(VkPhysicalDevice device, VkSurfaceKHR surface);
    
    VkExtent2D choose_swap_extent(Window const& window) const;
    VkSurfaceFormatKHR choose_format() const;
    VkPresentModeKHR choose_present_mode() const;
    uint32_t image_count() const;
    bool supported() const {
        return !formats.empty() and !present_modes.empty();
    }
};

struct Swapchain {
    // Needs to be recreated sometimes, it is a bit harder in it's own ctr, right? (Bad design)
    static Swapchain create(Device const& device, VkSurfaceKHR surface, Window const& window);

    void recreate(Device const& device, VkSurfaceKHR surface, Window const& window);
    void destroy(VkDevice device);
    
    SwapChainSupportDetails details;
    VkSwapchainKHR swapchain;
    VkFormat color_format;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
};

