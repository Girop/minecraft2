#include <glm/gtc/matrix_transform.hpp>
#include "uniforms.hpp"


glm::mat4 perspective_camera(VkExtent2D extent) 
{
    float aspect_ration {static_cast<float>(extent.width) / static_cast<float>(extent.height)};
    return glm::perspective(glm::radians(45.0f), aspect_ration, 0.1f, 10.0f);
}

constexpr glm::vec3 UP_DIRECTION(0.0f, 1.0f, 0.0f);

UniformBufferObject UniformBufferObject::current(VkExtent2D extent, glm::vec3 camera_position, glm::vec3 target_center)
{
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    glm::vec3 camera_direction = glm::normalize(camera_position - target_center);
    glm::vec3 camera_right = glm::normalize(glm::cross(UP_DIRECTION, camera_direction));
    glm::vec3 camera_up = glm::normalize(glm::cross(camera_direction, camera_right));

    ubo.view = glm::lookAt(camera_position, target_center, camera_up);
    ubo.projection = perspective_camera(extent);
    return ubo;
}

