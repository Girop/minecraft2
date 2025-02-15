#pragma once
#include <vulkan/vk_enum_string_helper.h>
#include <source_location>
#include "log.hpp"

namespace utils {

inline void check_vk(VkResult result, std::source_location const& loc = std::source_location::current()) 
{
    if (result == VK_SUCCESS) return;
    const auto msg = fmt::format("Vulkan operation failed: {}\n", string_VkResult(result));
    fail(msg, loc);
}

}
