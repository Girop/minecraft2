#pragma once
#include <filesystem>
#include <vulkan/vulkan.h>

VkShaderModule get_shader(VkDevice device, std::filesystem::path const& name);

