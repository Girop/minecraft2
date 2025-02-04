#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <vulkan/vk_enum_string_helper.h>
#include <vector>
#include <string>
#include "logging.hpp"

inline void check_vk(VkResult result, std::source_location loc = std::source_location::current()) 
{
    if (result == VK_SUCCESS) return;
    const auto msg = fmt::format("Vulkan operation failed: {}\n", string_VkResult(result));
    fail(msg, loc);
}

inline constexpr std::array activated_validation_layers {
    "VK_LAYER_KHRONOS_validation"
};

inline void verify_validation_layers() 
{
    uint32_t layerCount{};
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available_layers;
    available_layers.resize(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available_layers.data());
    
    bool all_found {true};

    for (auto& layer: activated_validation_layers) {
        auto found_it = std::ranges::find_if(available_layers, [&layer](auto& elem) {
            return std::strcmp(elem.layerName, layer) == 0;
        });
        
        fmt::print("Validation layer {}: ", layer);
        if (found_it != available_layers.end()) {
            fmt::println("FOUND");
        } else {
            fmt::println("MISSING");
            all_found = false;
        }
    }

    if (not all_found) {
        fail("Missing validation layers");
    }
}

