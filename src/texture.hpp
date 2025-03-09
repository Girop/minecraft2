#pragma once
#include <string>
#include "gfx/image.hpp"
#include "utils.hpp"


class Texture {
public:
    Texture(Device& device, std::string const& name);

    ~Texture();

    CONST_GETTER(image);
private:
    std::string name_;
    Image image_;
};
