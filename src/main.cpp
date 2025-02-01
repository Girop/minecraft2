#include "engine.hpp"

/* enum class Action { */
/*     Left, */
/*     Right, */
/*     Forward, */
/*     Backward, */
/*     Up, */
/*     Down, */
/* }; */
/*  */
/* static std::vector<Action> frame_actions; */
/* static const std::unordered_map<int, Action> mapping { */
/*     {GLFW_KEY_W, Action::Forward}, */
/*     {GLFW_KEY_S, Action::Backward}, */
/*     {GLFW_KEY_A, Action::Left}, */
/*     {GLFW_KEY_D, Action::Right}, */
/*     {GLFW_KEY_LEFT_SHIFT, Action::Down}, */
/*     {GLFW_KEY_SPACE, Action::Up}, */
/* }; */
/*  */
/* constexpr float camera_speed {0.4f}; */
/*  */
/*  */
/* void key_callback([[maybe_unused]] GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) */
/* { */
/*     if (action == GLFW_RELEASE) return; */
/*     if (mapping.find(key) == mapping.end()) return; */
/*     frame_actions.emplace_back(mapping.at(key)); */
/* } */


/* void log_gpu_info(VkPhysicalDevice device)  */
/* { */
/*     VkPhysicalDeviceProperties device_props; */
/*     vkGetPhysicalDeviceProperties(device, &device_props); */
/*     fmt::println("Name - {}", device_props.deviceName); */
/* } */
/*  */
/* void update_camera(glm::vec3& camera_pos) { */
/*     for (auto action : frame_actions) { */
/*         switch (action) { */
/*             case Action::Forward: */
/*                 camera_pos.z += camera_speed; */
/*                 break; */
/*             case Action::Backward: */
/*                 camera_pos.z -= camera_speed; */
/*                 break; */
/*             case Action::Left: */
/*                 camera_pos.x -= camera_speed; */
/*                 break; */
/*             case Action::Right: */
/*                 camera_pos.x += camera_speed; */
/*                 break; */
/*             case Action::Down: */
/*                 camera_pos.y -= camera_speed; */
/*                 break; */
/*             case Action::Up: */
/*                 camera_pos.y += camera_speed; */
/*                 break; */
/*         } */
/*     } */
/*     frame_actions.clear(); */
/*  */
/* } */

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) 
{
    Engine engine {}; 
    engine.run();
    engine.shutdown();
    return 0;
} 
