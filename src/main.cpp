#include <assert.h>
#include <algorithm>
#include <vulkan/vulkan.h>
#include <fmt/format.h>
#include <array>
#include <optional>
#include <vector>
#include <set>
#include "window.hpp"


VkInstance create_instance() {
    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    std::array extensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    std::array activated_validation_layers {
        "VK_LAYER_KHRONOS_validation"
    };

    uint32_t layerCount{};
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available_layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available_layers.data());
    
    for (auto& layer: activated_validation_layers) {
        auto found_it = std::ranges::find_if(available_layers, [&layer](auto& elem) {
            return std::strcmp(elem.layerName, layer) == 0;
        });
        assert(found_it != available_layers.end());
    }

    VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = activated_validation_layers.size(),
        .ppEnabledLayerNames = activated_validation_layers.data(),
        .enabledExtensionCount = extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkInstance instance;
    assert(vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS);
    return instance;
}


struct QueueFamilyIndicies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    static QueueFamilyIndicies from(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndicies indicies;

        uint32_t queue_famliy_count{};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_famliy_count, nullptr);
        assert(queue_famliy_count != 0);
        std::vector<VkQueueFamilyProperties> queue_families(queue_famliy_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_famliy_count, queue_families.data());

        int queue_idx{};
        for (auto const& queue: queue_families) {
            if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indicies.graphics = queue_idx;
            }

            VkBool32 supports_present {false};
            vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_idx, surface, &supports_present);
            if (supports_present) {
                indicies.present = queue_idx;
            }

            queue_idx++;
        }
        return indicies;
    }


    bool complete() const {
        return graphics.has_value() and present.has_value();
    }
};


constexpr std::array device_extensions {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


bool supports_extensions(VkPhysicalDevice device) {
    uint32_t extension_count{};

    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : extensions) {
        required_extensions.erase(extension.extensionName);
    }
     
    return required_extensions.empty();
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;

    static SwapChainSupportDetails query_from(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
    
    VkExtent2D choose_swap_extent() const {
        return capabilities.currentExtent; // FIXME: HERE MAY LAY A PROBLEM WITH COORDINATES
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


bool is_typical_gpu(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties device_props;
    VkPhysicalDeviceFeatures dev_features;

    vkGetPhysicalDeviceProperties(device, &device_props);
    vkGetPhysicalDeviceFeatures(device, &dev_features);

    return device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU and dev_features.geometryShader;
}


bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    bool extensisons_supported {supports_extensions(device)};
    bool matching_queues_present {QueueFamilyIndicies::from(device, surface).complete()};
    bool swapchain_complete {
        extensisons_supported 
            ? SwapChainSupportDetails::query_from(device, surface).supported() 
            : false
    };

    return is_typical_gpu(device)
        and extensisons_supported
        and swapchain_complete
        and matching_queues_present;
}


VkPhysicalDevice create_phy_device(VkInstance instance, VkSurfaceKHR surface) {
    VkPhysicalDevice chosen_device {VK_NULL_HANDLE};
    
    uint32_t device_count{0};
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    assert(device_count != 0);
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    for (auto const& device : devices) {
        if (is_device_suitable(device, surface)) {
            chosen_device = device;
            break;
        }
    }
    assert(chosen_device != VK_NULL_HANDLE);
    return chosen_device;
}


VkDevice create_logical_device(VkPhysicalDevice device, VkSurfaceKHR surface) {
    auto indicies = QueueFamilyIndicies::from(device, surface);
    std::set families {indicies.graphics.value(), indicies.present.value()};
    std::vector<VkDeviceQueueCreateInfo> create_infos;
    float queue_prio {1.0f};

    for (auto queue_family : families) {
        create_infos.emplace_back(
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, queue_family, 1, &queue_prio);
    }

    VkPhysicalDeviceFeatures dev_features{};

    VkDeviceCreateInfo device_create_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = static_cast<uint32_t>(create_infos.size()),
        .pQueueCreateInfos = create_infos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = device_extensions.size(),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &dev_features,
    };

    VkDevice logical_device{};
    assert(vkCreateDevice(device, &device_create_info, nullptr, &logical_device) == VK_SUCCESS);
    assert(logical_device != VK_NULL_HANDLE);
    return logical_device;
}


VkQueue get_graph_queue(VkDevice device, VkPhysicalDevice phys_device, VkSurfaceKHR surface) {
    auto queue_families {QueueFamilyIndicies::from(phys_device, surface)};

    VkQueue queue;
    vkGetDeviceQueue(device, queue_families.present.value(), 0, &queue);
    assert(queue != VK_NULL_HANDLE);
    return queue;
}


VkSurfaceKHR create_surface(HWND window, HINSTANCE instance, VkInstance vkinstance) {
    VkSurfaceKHR surface;

    VkWin32SurfaceCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .hinstance = instance,
        .hwnd = window,
    };

    assert(vkCreateWin32SurfaceKHR(vkinstance, &createInfo, nullptr, &surface) == VK_SUCCESS);
    assert(surface != VK_NULL_HANDLE);
    return surface;
}


struct SwapChainResult {
    VkSwapchainKHR handle;
    VkFormat used_format;
    VkExtent2D extent;
    std::vector<VkImage> images;
};

std::vector<VkImage> swapchain_images(VkSwapchainKHR swapchain, VkDevice device) {
    uint32_t image_count {};
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());
    return images;
}


SwapChainResult create_swapchain(
    SwapChainSupportDetails const& details,
    VkPhysicalDevice phys_device,
    VkDevice device,
    VkSurfaceKHR surface
) {
    auto format = details.choose_format();
    auto extent = details.choose_swap_extent();
    // If queues are separate use VK_SHARING_MODE_EXCLUSIVE
    auto indicies = QueueFamilyIndicies::from(phys_device, surface);
    std::array queue_indicies {indicies.graphics.value(), indicies.present.value()};

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
        .queueFamilyIndexCount = queue_indicies.size(),
        .pQueueFamilyIndices = queue_indicies.data(),
        .preTransform = details.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = details.choose_present_mode(),
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    VkSwapchainKHR swapchain;
    assert(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) == VK_SUCCESS);
    return {
        swapchain,
        format.format,
        extent,
        swapchain_images(swapchain, device)
    };
}


std::vector<VkImageView> create_imageviews(SwapChainResult const& swapchain, VkDevice device) {
    std::vector<VkImageView> views(swapchain.images.size());

    for (size_t idx{}; idx < swapchain.images.size(); ++idx) {
        VkImageViewCreateInfo info {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = swapchain.images[idx],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain.used_format,
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

        assert(vkCreateImageView(device, &info, nullptr, &views[idx]) == VK_SUCCESS);
    }
    return views;
}


constexpr std::string_view appname {"rendering engine"};

int WINAPI WinMain(
    [[maybe_unused]] HINSTANCE hinstance,
    [[maybe_unused]] HINSTANCE hPrevInstance, 
    [[maybe_unused]] LPSTR lpCmdLine,
    [[maybe_unused]] int nCmdShow)
{ 
    Window window {appname};

    VkInstance instance {create_instance()};
    VkSurfaceKHR surface {create_surface(window.get_window(), window.get_instance(), instance)};
    VkPhysicalDevice phys_device {create_phy_device(instance, surface)};
    VkDevice device {create_logical_device(phys_device, surface)};
    auto details = SwapChainSupportDetails::query_from(phys_device, surface);
    auto swapchain {create_swapchain(details, phys_device, device, surface)};
    auto image_views = create_imageviews(swapchain, device);

    // window.run();

    for (auto view : image_views) {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain.handle, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    fmt::print("Success\n");

    return 0;
} 
