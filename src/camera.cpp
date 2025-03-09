#include <vulkan/vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>
#include "camera.hpp"

PerspectiveCamera::PerspectiveCamera(glm::uvec2 const& extent):
    pitch{0.0f},
    yaw{0.0f},
    znear{0.001f},
    zfar{1000.f},
    aspect_rato{static_cast<float>(extent.x) / static_cast<float>(extent.y)},
    fov{glm::pi<float>() / 2.f},
    up_dir{0.f, 1.f, 0.f},
    position{0.f},
    target{0.f},
    view{0.f},
    projection{glm::perspective(fov, aspect_rato, znear, zfar)}
{}


constexpr float sensitivity {0.1f};

void PerspectiveCamera::update(glm::vec3 const& pos, glm::vec2 const& movement)
{
    position = pos;
    yaw += glm::radians(movement.x * sensitivity);
    yaw = fmodf(yaw, glm::two_pi<float>());

    pitch += glm::radians(movement.y * sensitivity);
    pitch = glm::clamp(pitch, glm::radians(-89.f), glm::radians(89.f));

    target = glm::normalize(glm::vec3{
        cosf(yaw) * cosf(pitch),
        sinf(pitch),
        cosf(pitch) * sinf(yaw)
    });
    view = glm::lookAt(position, position + target, up_dir);
}

