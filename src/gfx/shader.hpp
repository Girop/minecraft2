#pragma once
#include <vulkan/vulkan.h>
#include <string>

enum class ShaderType : uint8_t {
    Vertex,
    Fragment
};

class Device;

struct Shader {
    Shader(Device const& device, ShaderType const type, std::string const& name);

    std::string name;
    ShaderType type;
    VkShaderModule module;
};

