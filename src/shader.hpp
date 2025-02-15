#pragma once
#include <filesystem>
#include <vector>
#include <vulkan/vulkan.h>

enum class ShaderType {
    Vertex,
    Fragment
};

struct Shader {
    ShaderType type;
    VkShaderModule module;
};

class ShaderManager {
public:
    explicit ShaderManager(VkDevice device_);


    Shader vertex() const {
        return vertex_;
    }

    Shader fragment() const {
        return fragment_;
    }

private:
    Shader create_shader(ShaderType type) const;
    VkShaderModule compile(std::vector<std::byte> const& code) const;
    std::vector<std::byte> load(std::filesystem::path const& path) const;

    VkDevice device_; 
    Shader vertex_;
    Shader fragment_;
};
