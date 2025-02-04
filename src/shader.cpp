#include <fstream>
#include <unordered_map>
#include "shader.hpp"
#include "utility.hpp"

std::unordered_map<ShaderType, std::filesystem::path> const mapping {
    {ShaderType::Fragment, "build/shaders/triangle.frag.spv"},
    {ShaderType::Vertex, "build/shaders/triangle.vert.spv"},
};

Shader ShaderManager::create_shader(ShaderType type) const {
    auto const shader_bytes = load(mapping.at(type));
    auto const module = compile(shader_bytes);
    return {
        type, 
        module
    };
}

std::vector<std::byte> ShaderManager::load(std::filesystem::path const& path) const {
    std::ifstream fstream {path, std::ios::ate | std::ios::binary};

    if (!fstream.is_open()) {
        fail(fmt::format("Failed to open: {}", path.string()));
    }

    std::vector<std::byte> bytes(fstream.tellg());
    fstream.seekg(0);
    fstream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    return bytes;
}

VkShaderModule ShaderManager::compile(std::vector<std::byte> const& code) const {
    VkShaderModuleCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    check_vk(vkCreateShaderModule(device_, &info, nullptr, &module));
    return module;
}

ShaderManager::ShaderManager(VkDevice device_) :
    device_{device_},
    vertex_{create_shader(ShaderType::Vertex)},
    fragment_{create_shader(ShaderType::Fragment)}
{}
