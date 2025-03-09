#include <filesystem>
#include <vector>
#include <fstream>
#include "utils.hpp"
#include "device.hpp"
#include "shader.hpp"

namespace 
{
std::filesystem::path const prefix {"build/shaders"};
std::string const postfix {".spv"};

std::vector<std::byte> load(std::filesystem::path const& path) 
{
    std::ifstream fstream {path, std::ios::ate | std::ios::binary};
    if (!fstream.is_open()) 
    {
        fail("Failed to open: {}", path.string());
    }

    std::vector<std::byte> bytes(fstream.tellg());
    fstream.seekg(0);
    fstream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    return bytes;
}

VkShaderModule compile(Device const& device, std::vector<std::byte> const& code) 
{
    VkShaderModuleCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    utils::check_vk(vkCreateShaderModule(device.logical(), &info, nullptr, &module));
    return module;
}

} // namespace

Shader::Shader(Device const& device, ShaderType const type, std::string const& name) :
    name{name},
    type{type},
    module{compile(device, load(prefix / (name + postfix)))}
{}

