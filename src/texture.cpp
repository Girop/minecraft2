#include <stb_image.h>
#include <assert.h>
#include <filesystem>
#include "texture.hpp"
#include "gfx/buffer.hpp"

namespace 
{

std::filesystem::path const prefix {"res"};

Image load_texture(std::string const& name, Device& device) 
{
    auto const texture_path = prefix / name;
    debug("Loading texture: {}", texture_path.string());
    int x, y, chan;
    auto const* pixels = stbi_load(texture_path.string().c_str(), &x, &y, &chan, STBI_rgb_alpha);
    assert(pixels);

    VkFormat format {VK_FORMAT_R8G8B8A8_SRGB};
    VkExtent2D extent { static_cast<uint32_t>(x), static_cast<uint32_t>(y) };
    Image img 
    {
        device,
        format,
        extent,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    };

    size_t const size{x * y * 4u};
    img.fill(pixels, size);
    return img;
}

} // namespace


Texture::Texture(Device& device, std::string const& name):
    name_{name},
    image_{load_texture(name, device)}
{}

Texture::~Texture() = default;
