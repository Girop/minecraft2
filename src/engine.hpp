#pragma once
#include <GLFW/glfw3.h>
#include <span>
#include <functional>
#include "descriptors.hpp"
#include "viewport.hpp"
#include "window.hpp"
#include "device.hpp"
#include "shader.hpp"
#include "swapchain.hpp"
#include "pipeline.hpp"
#include "camera.hpp"
#include "buffer.hpp"
#include "uniforms.hpp"
#include "image.hpp"


class Engine {
    struct FrameData {
        VkCommandPool cmd_pool;
        VkCommandBuffer cmd_buffer; 
        VkSemaphore image_ready;
        VkSemaphore render_finished;
        VkFence frame_ready;
        VkFramebuffer framebuffer;
        UniformBuffer uniform_buffer;
        VkDescriptorSet descriptor_set; // TODO: Merge into uniforms
        float time_delta;
    };
    
    struct Queues {
        VkQueue graphics;
        VkQueue present;
    };

public:
    static constexpr auto NAME {"Mc2"};
    static constexpr uint32_t FRAME_OVERLAP {2};

    Engine();
    void shutdown();
    void run();
private:
    FrameData& current_frame() {
        return frame_data_[frame_number_ % FRAME_OVERLAP];
    }
    FrameData const& current_frame() const {
        return frame_data_[frame_number_ % FRAME_OVERLAP];
    }

    void prepare_framedata(uint32_t frame_index);
    void draw();

    UniformBuffer create_uniform_buf() const;
    Buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props) const;

    Image create_depth_image(VkFormat depth_format) const;

    void copy_buffer(Buffer const& source, Buffer& dst) const;
    void copy_buffer(void const* source, Buffer& dst) const;

    void destroy_buffer(Buffer& buffer) const;

    Buffer create_vertex_buf(std::span<Vertex const> verticies) const;
    Buffer create_index_buf(std::span<uint16_t const> indicies) const;

    void record(
        VkCommandBuffer cmd_buff,
        size_t count,
        VkDescriptorSet desc_set
    ) const;
    void submit(VkCommandBuffer cmd_buf, uint32_t image_index) const;

    void update(std::span<Action const> actions);

    Window window_;
    VkInstance instance_;
    VkSurfaceKHR surface_;
    Device device_;
    Swapchain swapchain_;
    VkExtent2D extent_;
    PerspectiveCamera camera_;
    Viewport viewport_;
    Queues queues_;
    ShaderManager shaders_;
    VkDescriptorSetLayout descriptor_set_layout_;
    VkDescriptorPool desc_pool_;
    VkRenderPass render_pass_;
    Pipeline pipeline_;
    std::vector<VkFramebuffer> frame_buffers_;
    std::array<FrameData, FRAME_OVERLAP> frame_data_;

    Buffer index_buffer_;
    Buffer vertex_buffer_;
    glm::vec3 player_position_ {-2.f, 0, 0};

    uint32_t frame_number_{0};
    std::vector<std::function<void()>> deletion_queue_;
};

