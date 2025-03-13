#include <span>
#include "renderer.hpp"
#include "uniforms.hpp"
#include "log.hpp"

namespace 
{

constexpr std::array activated_validation_layers {
    "VK_LAYER_KHRONOS_validation"
};

void verify_validation_layers() 
{
    uint32_t layerCount{};
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available_layers;
    available_layers.resize(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available_layers.data());
    for (auto const& layer: activated_validation_layers) {
        auto found_it = std::ranges::find_if(available_layers, [&layer](auto const& elem) {
            return std::strcmp(elem.layerName, layer) == 0;
        });
        if (found_it == available_layers.end()) 
        {
            fail("Missing validation layer: {}", layer);
        }
    }
}

VkInstance create_instance()
{
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Mc2",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    uint32_t extensionCount{};
    auto vulkan_extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    verify_validation_layers();
    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = activated_validation_layers.size(),
        .ppEnabledLayerNames = activated_validation_layers.data(),
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = vulkan_extensions,
    };

    VkInstance instance;
    utils::check_vk(vkCreateInstance(&createInfo, nullptr, &instance));
    return instance;
}

VkSurfaceKHR create_surface(VkInstance const instance, GLFWwindow* window)
{
    VkSurfaceKHR surface;
    utils::check_vk(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    return surface;
}

VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(
    bool const depth_test = true,
    bool const depth_write = true,
    VkCompareOp const comp_operator = VK_COMPARE_OP_LESS_OR_EQUAL)
{
    VkPipelineDepthStencilStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.depthTestEnable = depth_test ? VK_TRUE : VK_FALSE;
    info.depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE;
    info.depthCompareOp = depth_test ? comp_operator : VK_COMPARE_OP_ALWAYS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.minDepthBounds = 0.0f;
    info.maxDepthBounds = 1.0f;
    info.stencilTestEnable = VK_FALSE;
    return info;
}


Pipeline new_pipeline(
    Device const& device,
    Viewport const& viewport,
    std::span<Shader const> shaders,
    VkRenderPass const render_pass,
    VkDescriptorSetLayout const ubo,
    VkDescriptorSetLayout const texture)
{
    PipelineBuilder builder{};
    for (auto const& shader: shaders) 
    {
        builder = builder.set_shader(shader);
    }
    return builder
        .set_descriptor_sets(ubo, texture)
        .set_viewport(viewport)
        .set_render_pass(render_pass)
        .set_descriptions(Vertex::descriptions())
        .set_depth_testing(depth_stencil_create_info())
        .build(device.logical());
}

VkDescriptorSetLayout create_descriptor_set_layout(
    Device const& device, 
    VkDescriptorType const desc_type,
    VkShaderStageFlags const stage_flags)
{
    VkDescriptorSetLayoutBinding layout_binding 
    {
        .binding = 0,
        .descriptorType = desc_type,
        .descriptorCount = 1,
        .stageFlags = stage_flags,
        .pImmutableSamplers = nullptr,
    };
    

    VkDescriptorSetLayoutCreateInfo info 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &layout_binding,
    };

    VkDescriptorSetLayout descriptor_set_layout;
    utils::check_vk(vkCreateDescriptorSetLayout(device.logical(), &info, nullptr, &descriptor_set_layout));
    return descriptor_set_layout;
}

GpuBuffer index_buff(Device& device, std::span<uint16_t const> indicies)
{
    auto const buff_size = sizeof(uint16_t) * indicies.size();
    GpuBuffer transfer {
        device,
        buff_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    GpuBuffer target {
        device,
        buff_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    
    transfer.fill(indicies.data());
    target.copy_from(transfer);
    return target;
}

GpuBuffer vertex_buff(Device& device, std::span<Vertex const> verticies)
{
    
    auto const size = sizeof(Vertex) * verticies.size();
    GpuBuffer transfer 
    {
        device,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    GpuBuffer target 
    {
        device,
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    
    transfer.fill(verticies.data());
    target.copy_from(transfer);
    return target;
}

constexpr VkClearValue clear_color{{{0.f, 157.f / 256.f, 196.f / 256.f, 1.f}}};
constexpr VkClearValue clear_depth {.depthStencil = {.depth = 1.f, .stencil = 0}};
constexpr std::array clear_clrs {clear_color, clear_depth};

VkRenderPassBeginInfo render_pass_begin_info(
    VkRenderPass const render_pass, VkFramebuffer const fb, VkExtent2D const extent)
{
    VkRenderPassBeginInfo render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = render_pass,
        .framebuffer = fb,
        .renderArea = {.offset = {0, 0}, .extent = extent},
        .clearValueCount = clear_clrs.size(),
        .pClearValues = clear_clrs.data()
    };

    return render_pass_info;
}

VkSampler create_sampler(
    Device const& device,
    VkFilter const filter,
    VkSamplerAddressMode const address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT) 
{
    VkSamplerCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = filter;
    info.minFilter = filter;
    info.addressModeU = address_mode;
    info.addressModeV = address_mode;
    info.addressModeW = address_mode;

    VkSampler sampler;
    utils::check_vk(vkCreateSampler(device.logical(), &info, nullptr, &sampler));
    return sampler;
}

VkRenderPass new_pass(Device const& device, VkFormat const color_format, VkFormat const depth_format)
{
    VkAttachmentDescription color_attachement
    {
        .flags = 0,
        .format = color_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentDescription depth_attachment 
    {
        .flags = 0,
        .format = depth_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp= VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    std::array attachments {color_attachement, depth_attachment};

    VkAttachmentReference attachment_reference{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentReference depth_attachment_refrence 
    {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_refrence;

    VkSubpassDependency color_dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    VkSubpassDependency depth_dependency 
    {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    std::array dependencies {color_dependency, depth_dependency};
    VkRenderPassCreateInfo render_pass_info
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = dependencies.size(),
        .pDependencies = dependencies.data()
    };

    VkRenderPass render_pass;
    utils::check_vk(vkCreateRenderPass(device.logical(), &render_pass_info, nullptr, &render_pass));
    return render_pass;
}

std::vector<VkFramebuffer> new_frame_buffers(
    Device const& device,
    Swapchain const& swapchain,
    VkExtent2D extent,
    VkRenderPass renderpass,
    VkImageView depth_view
)
{
    size_t const image_count{swapchain.views.size()};
    std::vector<VkFramebuffer> frame_buffers;
    frame_buffers.resize(image_count);

    for (size_t idx{}; idx < image_count; ++idx)
    {
        std::array attachement{swapchain.views[idx], depth_view};
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderpass;
        info.attachmentCount = attachement.size();
        info.pAttachments = attachement.data();
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;
        utils::check_vk(vkCreateFramebuffer(device.logical(), &info, nullptr, &frame_buffers[idx]));
    }
    return frame_buffers;
}


} // namespace

Renderer::Renderer(Window& window) :
    window_{window},
    instance_{create_instance()},
    surface_{create_surface(instance_, window_.handle())},
    device_{instance_, surface_},
    swapchain_{Swapchain::create(device_, surface_, window_)},
    extent_{swapchain_.details.capabilities.currentExtent},
    viewport_{extent_},
    queue_{[&] {
        QueueFamily family {device_.physical(), surface_};
        return Queues{family, family.get_queue(device_.logical())};
    }()},
    shaders_{Shader{device_, ShaderType::Fragment, "cube.frag"}, Shader{device_, ShaderType::Vertex, "cube.vert"}},
    ubo_layout_{create_descriptor_set_layout(device_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)},
    texture_layout_{create_descriptor_set_layout(device_, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)},
    descriptor_pool_{device_, FRAME_OVERLAP},
    render_pass_{new_pass(device_, swapchain_.color_format, VK_FORMAT_D32_SFLOAT)},
    main_pipeline_{new_pipeline(device_, viewport_, shaders_, render_pass_, ubo_layout_, texture_layout_)},
    sampler_{create_sampler(device_, VK_FILTER_NEAREST)},
    depth_{device_, VK_FORMAT_D32_SFLOAT, extent_, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT},
    frame_buffers_{new_frame_buffers(device_, swapchain_, extent_, render_pass_, depth_.view())},
    texture_{device_, "dirt.png"},
    frames_{
        device_,
        surface_,
        descriptor_pool_.allocate_descriptor_sets(texture_layout_, FRAME_OVERLAP),
        descriptor_pool_.allocate_descriptor_sets(ubo_layout_, FRAME_OVERLAP),
        texture_,
        sampler_
    }
{
    info("Renderer intialized");
}

Renderer::~Renderer() 
{
    device_.wait();
}

void Renderer::handle_world_data(RenderData const& data) 
{
    auto& frame = current_frame();
    frame.cmd.reset();
    UniformBufferObject const ubo 
    {
        glm::mat4(1.f),
        data.camera.view,
        data.camera.projection
    };
    frame.uniform.fill(reinterpret_cast<void const*>(&ubo));
}

std::optional<uint32_t> Renderer::acquire_image()
{
    uint32_t image_index{};
    auto const result = vkAcquireNextImageKHR(
        device_.logical(),
        swapchain_.swapchain,
        UINT64_MAX,
        current_frame().image_acquired.handle(),
        VK_NULL_HANDLE,
        &image_index
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR)
    {
        swapchain_.recreate(device_, surface_, window_);
        return std::nullopt;
    }
    utils::check_vk(result);
    return {image_index};
}


// Need some chunk data bookkeeping, given some chunks,
// e.g check if data is already allocated otherwiser setup GPU resources
void Renderer::draw(RenderData const& render_data) 
{
    // For now:
    static GpuBuffer index{index_buff(device_, render_data.indices)};
    static GpuBuffer vertex{vertex_buff(device_, render_data.vertices)};
    static size_t count{render_data.indices.size()};

    auto& frame = current_frame();
    frame.cmd.wait();
    handle_world_data(render_data);
    auto const swapchain_index = acquire_image();
    if (not swapchain_index.has_value()) return;

    record(*swapchain_index, count, index, vertex);
    submit();
    present(*swapchain_index);
    ++frame_number_;
}

void Renderer::record(uint32_t const swapchain_index, uint32_t const index_count, GpuBuffer const& indicies, GpuBuffer const& verticies)
{
    auto& frame = current_frame();
    frame.cmd.record([&](VkCommandBuffer cmd) {
        auto begin_info = render_pass_begin_info(render_pass_, frame_buffers_.at(swapchain_index), extent_);
        vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline_.pipeline);
        vkCmdSetViewport(cmd, 0, 1, &viewport_.viewport);
        vkCmdSetScissor(cmd, 0, 1, &viewport_.scissors);
        VkDeviceSize offset{0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &verticies.handle(), &offset);
        vkCmdBindIndexBuffer(cmd, indicies.handle(), 0, VK_INDEX_TYPE_UINT16);
        
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline_.layout, 0, 1, &frame.unfirom_descriptor, 0, nullptr);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, main_pipeline_.layout, 1, 1, &frame.texture_descriptor, 0, nullptr);

        vkCmdDrawIndexed(cmd, index_count, 1, 0, 0, 0);
        vkCmdEndRenderPass(cmd);
    });
}

void Renderer::submit()
{
    auto& frame = current_frame();
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.image_acquired.handle(),
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame.cmd.buffer(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &frame.image_rendered.handle()
    };
    assert(!frame.cmd.execution_fence().is_signaled());
    frame.cmd.submit(submit_info);
}

void Renderer::present(uint32_t const& swapchain_index)
{
    auto& frame = current_frame();
    VkPresentInfoKHR present_info
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.image_rendered.handle(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain_.swapchain,
        .pImageIndices = &swapchain_index,
        .pResults = nullptr
    };
    utils::check_vk(vkQueuePresentKHR(queue_.queue, &present_info));
}
