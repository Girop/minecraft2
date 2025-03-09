#pragma once
#include "interfaces.hpp"
#include "device.hpp"
#include "window.hpp"
#include "swapchain.hpp"
#include "viewport.hpp"
#include "shader.hpp"
#include "queues.hpp"
#include "pipeline.hpp"
#include "buffer.hpp"
#include "image.hpp"
#include "framedata.hpp"


class Renderer {
public:
    Renderer(Window& window);

    Renderer(Renderer const&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer const&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    ~Renderer();

    void draw(RenderData const& render_data);
private:
    void handle_world_data(RenderData const& data);

    [[nodiscard]] Framedata& current_frame()
    {
        return frames_.data[frame_number_ % FRAME_OVERLAP];
    }

    [[nodiscard]] Framedata const& current_frame() const
    {
        return frames_.data[frame_number_ % FRAME_OVERLAP];
    }
    
    std::optional<uint32_t> acquire_image();
    void record(uint32_t const swapchain_index, uint32_t const index_count, GpuBuffer const& indicies, GpuBuffer const& verticies);
    void submit();
    void present(uint32_t const& swapchain_index);

    Window& window_;
    VkInstance instance_;
    VkSurfaceKHR surface_;
    Device device_;
    Swapchain swapchain_;
    VkExtent2D const& extent_;
    Viewport viewport_;
    struct Queues {
        QueueFamily family;
        VkQueue queue;
    } queue_;
    std::array<Shader, 2> shaders_;
    VkDescriptorSetLayout ubo_layout_;
    VkDescriptorSetLayout texture_layout_;
    DescriptorPool descriptor_pool_;
    VkRenderPass render_pass_;
    Pipeline main_pipeline_; 
    VkSampler sampler_;
    Image depth_;
    std::vector<VkFramebuffer> frame_buffers_;
    Texture texture_;
    Frames frames_;

    size_t frame_number_{};
};

