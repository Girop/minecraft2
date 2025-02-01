#include <vector>
#include <fstream>
#include "shader.hpp"
#include "utility.hpp"


std::vector<std::byte> load_shader(std::filesystem::path const& path) {
    std::ifstream fstream {path, std::ios::ate | std::ios::binary};

    if (!fstream.is_open()) {
        fmt::println("Failed to open: {}", path.string());
        abort();
    }

    std::vector<std::byte> bytes(fstream.tellg());
    fstream.seekg(0);
    fstream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    return bytes;
}

VkShaderModule create_shader_module(VkDevice device, std::vector<std::byte> const& code) {
    VkShaderModuleCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    VkShaderModule module;
    check_vk(vkCreateShaderModule(device, &info, nullptr, &module));
    return module;
}

VkShaderModule get_shader(VkDevice device, std::filesystem::path const& name) {
    auto shader_bytes = load_shader(name);
    return create_shader_module(device, shader_bytes);
}

