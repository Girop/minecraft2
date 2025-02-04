#include "engine.hpp"
#include "descriptors.hpp"
#include "queues.hpp"
#include "shader.hpp"
#include "swapchain.hpp"
#include "utility.hpp"
#include <fmt/format.h>
#include <ranges>

namespace init
{

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
    check_vk(vkCreateInstance(&createInfo, nullptr, &instance));
    return instance;
}

VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    check_vk(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    return surface;
}

VkFence create_signaled_fence(VkDevice device)
{
    VkFenceCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    VkFence fence;
    check_vk(vkCreateFence(device, &info, nullptr, &fence));
    return fence;
}

VkSemaphore create_semaphore(VkDevice device)
{
    VkSemaphoreCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VkSemaphore sem;
    check_vk(vkCreateSemaphore(device, &info, nullptr, &sem));
    return sem;
}

VkCommandPool create_command_pool(Device const& device, VkSurfaceKHR surface)
{
    auto queue_families = QueueFamilyIndicies::from(device.physical, surface);
    VkCommandPoolCreateInfo info{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                 .pNext = nullptr,
                                 .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                 .queueFamilyIndex = queue_families.graphics.value()};
    VkCommandPool comand_pool;
    check_vk(vkCreateCommandPool(device.logical, &info, nullptr, &comand_pool));
    return comand_pool;
}

VkCommandBuffer allocate_command_buffer(VkDevice device, VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer buffer;
    check_vk(vkAllocateCommandBuffers(device, &info, &buffer));

    return buffer;
}

VkRenderPass create_render_pass(VkDevice device, VkFormat color_format, VkFormat depth_format)
{
    VkAttachmentDescription color_attachement{
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

    VkAttachmentDescription depth_attachment {
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

    std::array attachments{color_attachement, depth_attachment};

    VkAttachmentReference attachment_reference{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentReference depth_attachment_refrence {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_refrence;

    VkSubpassDependency color_dependency{.srcSubpass = VK_SUBPASS_EXTERNAL,
                                         .dstSubpass = 0,
                                         .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                         .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                         .srcAccessMask = 0,
                                         .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                         .dependencyFlags = 0};

    VkSubpassDependency depth_dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    std::array dependencies {color_dependency, depth_dependency};
    VkRenderPassCreateInfo render_pass_info{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                            .pNext = nullptr,
                                            .flags = 0,
                                            .attachmentCount = attachments.size(),
                                            .pAttachments = attachments.data(),
                                            .subpassCount = 1,
                                            .pSubpasses = &subpass,
                                            .dependencyCount = dependencies.size(),
                                            .pDependencies = dependencies.data()};

    VkRenderPass render_pass;
    check_vk(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass));
    return render_pass;
}

constexpr VkClearValue clear_color{{{0.f, 0.f, 0.f, 1.f}}};
constexpr VkClearValue clear_depth {.depthStencil = {.depth = 1.f, .stencil = 0}};
constexpr std::array clear_clrs {clear_color, clear_depth};

VkRenderPassBeginInfo render_pass_begin_info(VkRenderPass render_pass, VkFramebuffer fb, VkExtent2D extent)
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

VkDescriptorSetLayout create_descriptor_set_layout(VkDevice device)
{
    VkDescriptorSetLayoutBinding ubo_layout_binding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &ubo_layout_binding,
    };

    VkDescriptorSetLayout descriptor_set_layout;
    check_vk(vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptor_set_layout));
    return descriptor_set_layout;
}

VkDescriptorPool create_descriptor_pool(VkDevice device, uint32_t frame_count)
{
    VkDescriptorPoolSize size{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = frame_count,
    };

    VkDescriptorPoolCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = frame_count,
        .poolSizeCount = 1,
        .pPoolSizes = &size,
    };

    VkDescriptorPool pool;
    check_vk(vkCreateDescriptorPool(device, &info, nullptr, &pool));
    return pool;
}

template <uint32_t FRAME_COUNT>
std::array<VkDescriptorSet, FRAME_COUNT> allocate_descriptor_sets(VkDevice device, VkDescriptorSetLayout layout,
                                                                  VkDescriptorPool pool)
{
    std::array<VkDescriptorSetLayout, FRAME_COUNT> layouts{layout, layout};
    VkDescriptorSetAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                           .pNext = nullptr,
                                           .descriptorPool = pool,
                                           .descriptorSetCount = FRAME_COUNT,
                                           .pSetLayouts = layouts.data()};

    std::array<VkDescriptorSet, FRAME_COUNT> descriptor_sets;
    check_vk(vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()));
    return descriptor_sets;
}

void fill_descriptor_set(VkDevice device, VkDescriptorSet set, UniformBuffer const &ub)
{
    VkDescriptorBufferInfo buffer_info{.buffer = ub.buffer.buffer, .offset = 0, .range = sizeof(UniformBufferObject)};

    VkWriteDescriptorSet desc_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &buffer_info,
        .pTexelBufferView = nullptr,
    };
    vkUpdateDescriptorSets(device, 1, &desc_write, 0, nullptr);
}


VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp)
{
    VkPipelineDepthStencilStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.depthTestEnable = bDepthTest ? VK_TRUE : VK_FALSE;
    info.depthWriteEnable = bDepthWrite ? VK_TRUE : VK_FALSE;
    info.depthCompareOp = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.minDepthBounds = 0.0f;
    info.maxDepthBounds = 1.0f;
    info.stencilTestEnable = VK_FALSE;
    return info;
}

VkImageCreateInfo image_create_info(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage) {
    VkImageCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = format;
    create_info.extent = {extent.width, extent.height, 1};
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;
    return create_info;
}

VkImageViewCreateInfo image_view_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspects) 
{
    VkImageViewCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.format = format;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.subresourceRange = {
        .aspectMask = aspects,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    return info;
}

std::vector<VkFramebuffer> create_frame_buffers(
    VkDevice device,
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
        check_vk(vkCreateFramebuffer(device, &info, nullptr, &frame_buffers[idx]));
    }
    return frame_buffers;
}

VkExtent2D create_extent(VkSurfaceCapabilitiesKHR const& capabilities, glm::uvec2 const& winsize)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    auto const &min_extent = capabilities.minImageExtent;
    auto const &max_extent = capabilities.maxImageExtent;
    return {
        .width = std::clamp(winsize.x, min_extent.width, max_extent.width),
        .height = std::clamp(winsize.y, min_extent.width, max_extent.width),
    };
}

} // namespace init

constexpr glm::vec3 red{1.f, 0.f, 0.f};
constexpr glm::vec3 blue{0.f, 0.f, 1.f};
constexpr glm::vec3 green{0.f, 1.f, 0.f};

constexpr float p {0.5f};
const std::vector<Vertex> verticies_back{
    {{-p, -p, -p}, red},  // 0
    {{-p, p, -p}, green}, // 1
    {{p, p, -p}, blue},   // 2
    {{p, -p, -p}, red},   // 3
    {{p, -p, p}, green},  // 4
    {{p, p, p}, blue},    // 5
    {{-p, p, p}, red},    // 6
    {{-p, -p, p}, green}, // 7
};

const std::vector<uint16_t> indices{
    0, 2, 1, 0, 3, 2, // front
    3, 2, 4, 5, 4, 2, // right
    1, 0, 6, 0, 7, 6, // left
    1, 2, 6, 2, 6, 5, // up
    0, 3, 7, 3, 7, 4, // down
    7, 4, 5, 7, 5, 6, // back
};

uint32_t find_memory_type_idx(VkPhysicalDevice device, uint32_t type_filter, VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties memory_props;
    vkGetPhysicalDeviceMemoryProperties(device, &memory_props);
    uint32_t res{0};
    for (uint32_t idx{0}; idx < memory_props.memoryTypeCount; ++idx)
    {
        bool prop_pattern_matches = (memory_props.memoryTypes[idx].propertyFlags & props) == props;
        bool type_matches = type_filter & (1 << idx);

        if (prop_pattern_matches and type_matches)
        {
            res = idx;
            break;
        }
    }
    assert(res != 0);
    return res;
}

constexpr VkFormat depth_format {VK_FORMAT_D32_SFLOAT};



Engine::Engine()
    : window_{NAME},
    instance_{init::create_instance()},
    surface_{init::create_surface(instance_, window_.handle())},
    device_{Device::create(instance_, surface_)},
    swapchain_{Swapchain::create(device_, surface_, window_)},
    extent_{init::create_extent(swapchain_.details.capabilities, window_.size())},
    viewport_{extent_},
    queues_{[&]() {
        auto const queue_families = QueueFamilyIndicies::from(device_.physical, surface_);
        auto const graphics_queue = queue_families.graphics_queue(device_.logical);
        auto const present_queue = queue_families.present_queue(device_.logical);
        return Queues{graphics_queue, present_queue};
    }()},
    shaders_{device_.logical},
    descriptor_set_layout_{init::create_descriptor_set_layout(device_.logical)},
    desc_pool_{init::create_descriptor_pool(device_.logical, FRAME_OVERLAP)},
    render_pass_{init::create_render_pass(device_.logical, swapchain_.color_format, depth_format)},
    pipeline_{[&]() {
        auto const binding_desc = binding_description();
        auto const attribute_desc = attribute_descritptions();
        auto const depth = init::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

        PipelineBuilder builder{};
        return builder
            .set_viewport(viewport_)
            .set_fragment(shaders_.fragment().module)
            .set_vertex(shaders_.vertex().module)
            .set_render_pass(render_pass_)
            .set_descriptors(binding_desc, attribute_desc)
            .set_depth_testing(depth)
            .build(device_.logical);
    }()},
    frame_buffers_{init::create_frame_buffers(
        device_.logical, swapchain_, extent_, render_pass_, create_depth_image(depth_format).image_view)},
    frame_data_{[&]() {
        std::array<FrameData, FRAME_OVERLAP> data;
        auto const desc_sets = init::allocate_descriptor_sets<FRAME_OVERLAP>(device_.logical, descriptor_set_layout_, desc_pool_);

        uint32_t idx{};
        for (auto &frame : frame_data_)
        {
            auto cmd_pool_ = init::create_command_pool(device_, surface_);
            frame.cmd_pool = cmd_pool_;
            frame.cmd_buffer = init::allocate_command_buffer(device_.logical, cmd_pool_);
            frame.render_finished = init::create_semaphore(device_.logical);
            frame.image_ready = init::create_semaphore(device_.logical);
            frame.frame_ready = init::create_signaled_fence(device_.logical);
            frame.uniform_buffer = create_uniform_buf();

            frame.descriptor_set = desc_sets.at(idx);
            init::fill_descriptor_set(device_.logical, frame.descriptor_set, frame.uniform_buffer);
            idx++;
        }

        return data;
    }()},
    index_buffer_{create_index_buf(indices)},
    vertex_buffer_{create_vertex_buf(verticies_back)}
{}

Image Engine::create_depth_image(VkFormat depth_format) const {
    auto const depth_image_info = init::image_create_info(depth_format, extent_, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkImage depth_image;
    check_vk(vkCreateImage(device_.logical, &depth_image_info, nullptr, &depth_image));

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device_.logical, depth_image, &mem_reqs);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = find_memory_type_idx(device_.physical, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory mem;
    check_vk(vkAllocateMemory(device_.logical, &alloc_info, nullptr, &mem));
    check_vk(vkBindImageMemory(device_.logical, depth_image, mem, 0));

    auto const depth_img_view_info = init::image_view_create_info(depth_format, depth_image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VkImageView depth_image_view;
    check_vk(vkCreateImageView(device_.logical, &depth_img_view_info, nullptr, &depth_image_view));

    return {depth_image, depth_image_view, mem};
}

Buffer Engine::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props) const
{
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Buffer buffer;
    check_vk(vkCreateBuffer(device_.logical, &create_info, nullptr, &buffer.buffer));

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device_.logical, buffer.buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                    .pNext = nullptr,
                                    .allocationSize = mem_reqs.size,
                                    .memoryTypeIndex =
                                        find_memory_type_idx(device_.physical, mem_reqs.memoryTypeBits, props)};

    check_vk(vkAllocateMemory(device_.logical, &alloc_info, nullptr, &buffer.memory));
    vkBindBufferMemory(device_.logical, buffer.buffer, buffer.memory, 0);
    buffer.size = size;
    return buffer;
}

UniformBuffer Engine::create_uniform_buf() const
{
    constexpr size_t ubo_size{sizeof(UniformBufferObject)};
    UniformBuffer ub;
    ub.buffer = create_buffer(ubo_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkMapMemory(device_.logical, ub.buffer.memory, 0, ubo_size, 0, &ub.mapped);
    return ub;
}

void Engine::destroy_buffer(Buffer &buffer) const
{
    vkDestroyBuffer(device_.logical, buffer.buffer, nullptr);
    vkFreeMemory(device_.logical, buffer.memory, nullptr);
    buffer.size = 0;
}

void Engine::copy_buffer(void const *source, Buffer &dst) const
{
    void *data;
    vkMapMemory(device_.logical, dst.memory, 0, dst.size, 0, &data);
    memcpy(data, source, dst.size);
    vkUnmapMemory(device_.logical, dst.memory);
}

void Engine::copy_buffer(Buffer const &source, Buffer &dst) const
{
    auto const &frame = current_frame();
    auto cmd_buffer = init::allocate_command_buffer(device_.logical, frame.cmd_pool);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd_buffer, &begin_info);
    VkBufferCopy copy{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = source.size,
    };

    vkCmdCopyBuffer(cmd_buffer, source.buffer, dst.buffer, 1, &copy);
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;

    vkQueueSubmit(queues_.graphics, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(device_.logical); // TODO: futures?

    vkFreeCommandBuffers(device_.logical, frame.cmd_pool, 1, &cmd_buffer);
}

void Engine::record(VkCommandBuffer cmd_buff, size_t count, VkDescriptorSet desc_set) const
{
    auto &frame = current_frame();

    VkCommandBufferBeginInfo buffer_begin_info{};
    buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd_buff, &buffer_begin_info);

    auto rpb_info = init::render_pass_begin_info(render_pass_, frame.framebuffer, extent_);
    vkCmdBeginRenderPass(cmd_buff, &rpb_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.pipeline);
    vkCmdSetViewport(cmd_buff, 0, 1, &viewport_.viewport);
    vkCmdSetScissor(cmd_buff, 0, 1, &viewport_.scissors);
    VkDeviceSize offset{0};
    vkCmdBindVertexBuffers(cmd_buff, 0, 1, &vertex_buffer_.buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buff, index_buffer_.buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout, 0, 1, &desc_set, 0, nullptr);
    vkCmdDrawIndexed(cmd_buff, count, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd_buff);
    check_vk(vkEndCommandBuffer(cmd_buff));
}

void Engine::prepare_framedata(uint32_t framebuffer_idx)
{
    auto& frame = current_frame();
    frame.framebuffer = frame_buffers_.at(framebuffer_idx);
    frame.time_delta = glfwGetTime();
    vkResetCommandBuffer(frame.cmd_buffer, 0);
}

void Engine::run()
{
    while (not window_.should_close())
    {
        auto const actions = window_.collect_actions();
        update_camera(actions);
        draw();
        ++frame_number_;
    }
}

constexpr glm::vec3 target{0.f, 0.f, 0.f};
constexpr float camera_speed{0.1};

void Engine::update_camera(std::span<Action const> actions)
{
    float delta {camera_speed * current_frame().time_delta};
    for (auto const action : actions)
    {
        switch (action)
        {
        case Action::Forward:
            camera_pos_.z += delta;
            break;
        case Action::Backward:
            camera_pos_.z -= delta;
            break;
        case Action::Left:
            camera_pos_.x -= delta;
            break;
        case Action::Right:
            camera_pos_.x += delta;
            break;
        case Action::Down:
            camera_pos_.y -= delta;
            break;
        case Action::Up:
            camera_pos_.y += delta;
            break;
        }
    }
}

void Engine::draw()
{
    auto &frame = current_frame();
    check_vk(vkWaitForFences(device_.logical, 1, &frame.frame_ready, VK_TRUE, 1000000000));
    vkResetFences(device_.logical, 1, &frame.frame_ready);

    uint32_t image_index{};
    VkResult acquired_image{
        vkAcquireNextImageKHR(
            device_.logical,
            swapchain_.swapchain,
            UINT64_MAX,
            frame.image_ready,
            VK_NULL_HANDLE,
            &image_index)
    };
    if (acquired_image == VK_ERROR_OUT_OF_DATE_KHR or acquired_image == VK_SUBOPTIMAL_KHR)
    {
        // TODO: Might not resize, handle it in the GLFW window too
        swapchain_.recreate(device_, surface_, window_);
        return;
    }

    check_vk(acquired_image);
    prepare_framedata(image_index);
    frame.uniform_buffer.copy(UniformBufferObject::current(extent_, camera_pos_, target));
    record(frame.cmd_buffer, indices.size(), frame.descriptor_set);
    submit(frame.cmd_buffer, image_index);
}

void Engine::submit(VkCommandBuffer cmd_buf, uint32_t image_index) const
{
    auto const &frame = current_frame();

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             .pNext = nullptr,
                             .waitSemaphoreCount = 1,
                             .pWaitSemaphores = &frame.image_ready,
                             .pWaitDstStageMask = wait_stages,
                             .commandBufferCount = 1,
                             .pCommandBuffers = &cmd_buf,
                             .signalSemaphoreCount = 1,
                             .pSignalSemaphores = &frame.render_finished};
    check_vk(vkQueueSubmit(queues_.graphics, 1, &submit_info, frame.frame_ready));
    VkPresentInfoKHR present_info{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                  .pNext = nullptr,
                                  .waitSemaphoreCount = 1,
                                  .pWaitSemaphores = &frame.render_finished,
                                  .swapchainCount = 1,
                                  .pSwapchains = &swapchain_.swapchain,
                                  .pImageIndices = &image_index,
                                  .pResults = nullptr};
    check_vk(vkQueuePresentKHR(queues_.present, &present_info));
}

Buffer Engine::create_vertex_buf(std::span<Vertex const> verticies) const
{
    size_t const buffer_size{verticies.size() * sizeof(verticies[0])};
    auto transfer_buf = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    copy_buffer(verticies.data(), transfer_buf);

    auto vertex_buffer = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    copy_buffer(transfer_buf, vertex_buffer);
    destroy_buffer(transfer_buf);
    return vertex_buffer;
}

Buffer Engine::create_index_buf(std::span<uint16_t const> indicies) const
{
    size_t const buffer_size{indicies.size() * sizeof(indicies[0])};
    auto transfer_buffer = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    auto index_buffer = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    copy_buffer(indicies.data(), transfer_buffer);
    copy_buffer(transfer_buffer, index_buffer);
    destroy_buffer(transfer_buffer);
    return index_buffer;
}

void Engine::shutdown()
{
    vkDeviceWaitIdle(device_.logical);
    for (auto const& deleter : deletion_queue_ | std::views::reverse)
    {
        deleter();
    }
    glfwTerminate();
}
