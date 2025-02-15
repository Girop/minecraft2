#pragma once
#include <glm/glm.hpp>

struct PerspectiveCamera {
    PerspectiveCamera(struct VkExtent2D const& extent);

    void update(glm::vec3 const& pos, glm::vec2 const& delta);

    float pitch, yaw, znear, zfar, aspect_rato, fov;
    glm::vec3 up_dir, position, target;
    glm::mat4 view, projection;
};

