#include <chrono>
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
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), elapsed * glm::radians(90.0f), glm::vec3(0.f, 1.f, 0.f));
    glm::vec3 camera_direction = glm::normalize(camera_position - target_center);
    glm::vec3 camera_right = glm::normalize(glm::cross(UP_DIRECTION, camera_direction));
    glm::vec3 camera_up = glm::normalize(glm::cross(camera_direction, camera_right));

    ubo.view = glm::lookAt(camera_position, target_center, camera_up);
    ubo.projection = perspective_camera(extent);
    return ubo;
}

