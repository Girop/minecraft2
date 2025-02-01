#include <ranges>
#include <set>
#include <fmt/format.h>
#include "descriptors.hpp"
#include "shader.hpp"
#include "queues.hpp"
#include "swapchain.hpp"
#include "engine.hpp"
#include "utility.hpp"


namespace init {

VkInstance create_instance()
{
    VkApplicationInfo appInfo {
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
    VkInstanceCreateInfo createInfo {
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

VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow* window) {
    VkSurfaceKHR surface;
    check_vk(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    return surface;
}


VkDeviceCreateInfo device_create_info(std::vector<VkDeviceQueueCreateInfo> const& create_infos);


class DeviceFinder {
public: 

    static constexpr std::array DEVICE_EXTENSIONS {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    DeviceFinder(VkInstance instance, VkSurfaceKHR surface):
        instance_{instance},
        surface_{surface} 
    {}

    VkPhysicalDevice find() const {
        uint32_t device_count{};
        vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
        std::vector<VkPhysicalDevice> devices;
        devices.resize(device_count);
        vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
        VkPhysicalDevice device;
        for (auto phys_device : devices) {
            if (!suitable(phys_device)) continue;
            device = phys_device;
            break;
        }
        assert(device != VK_NULL_HANDLE);
        return device;
    }

    VkDevice logical(VkPhysicalDevice phys_device) const {
        auto indicies = QueueFamilyIndicies::from(phys_device, surface_);
        std::array families {indicies.graphics.value(), indicies.present.value()};
        std::vector<VkDeviceQueueCreateInfo> create_infos;
        create_infos.resize(families.size());

        float queue_prio {1.0f};
        for (size_t idx{}; idx < create_infos.size(); ++idx) {
            create_infos[idx] = {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                nullptr,
                0,
                families[idx],
                1,
                &queue_prio
            };
        }

        auto device_info = device_create_info(create_infos);
        VkDevice device;
        check_vk(vkCreateDevice(phys_device, &device_info, nullptr, &device));
        return device;
    }

private:
    bool suitable(VkPhysicalDevice phys_device) const {
        return supports_extensions(phys_device) 
            and is_external_gpu(phys_device)
            and QueueFamilyIndicies::from(phys_device, surface_).complete()
            and SwapChainSupportDetails::create(phys_device, surface_).supported();
    }

    bool is_external_gpu(VkPhysicalDevice device) const {
        VkPhysicalDeviceProperties device_props;
        VkPhysicalDeviceFeatures dev_features;

        vkGetPhysicalDeviceProperties(device, &device_props);
        vkGetPhysicalDeviceFeatures(device, &dev_features);

        return (
                device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU or
                device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU  
        ) and dev_features.geometryShader;
    }


    bool supports_extensions(VkPhysicalDevice device) const {
        uint32_t extension_count{};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        
        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

        std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
        for (const auto& extension : extensions) {
            required_extensions.erase(extension.extensionName);
        }
        return required_extensions.empty();
    }
 
    VkInstance instance_; 
    VkSurfaceKHR surface_;
};

VkDeviceCreateInfo device_create_info(std::vector<VkDeviceQueueCreateInfo> const& create_infos) {
    VkPhysicalDeviceFeatures dev_features{};
    dev_features.fillModeNonSolid = VK_TRUE;
     
    VkDeviceCreateInfo device_create_info {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(create_infos.size());
    device_create_info.pQueueCreateInfos = create_infos.data();
    device_create_info.enabledExtensionCount = DeviceFinder::DEVICE_EXTENSIONS.size();
    device_create_info.ppEnabledExtensionNames = DeviceFinder::DEVICE_EXTENSIONS.data();
    device_create_info.pEnabledFeatures = &dev_features;

    return device_create_info;
}

VkExtent2D extent(VkSurfaceCapabilitiesKHR caps, glm::uvec2 const& winsize) {
    auto const& current_extent = caps.currentExtent;

    if (current_extent.width != std::numeric_limits<uint32_t>::max()) {
        return current_extent;
    }
    auto const& min_extent = caps.minImageExtent;
    auto const& max_extent = caps.maxImageExtent;
    return {
        .width = std::clamp(winsize.x, min_extent.width, max_extent.width),
        .height = std::clamp(winsize.y, min_extent.width, max_extent.width),
    };
}

VkFence create_signaled_fence(VkDevice device) {
    VkFenceCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    VkFence fence;
    check_vk(vkCreateFence(device, &info, nullptr, &fence));
    return fence;
}

VkSemaphore create_semaphore(VkDevice device) {
    VkSemaphoreCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VkSemaphore sem;
    check_vk(vkCreateSemaphore(device, &info, nullptr, &sem));
    return sem;
}

VkCommandPool create_command_pool(VkPhysicalDevice phy_device, VkDevice device, VkSurfaceKHR surface) {
    auto queue_families = QueueFamilyIndicies::from(phy_device, surface);
    VkCommandPoolCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_families.graphics.value()
    };
    VkCommandPool comand_pool;
    check_vk(vkCreateCommandPool(device, &info, nullptr, &comand_pool));
    return comand_pool;
}

VkCommandBuffer allocate_command_buffer(VkDevice device, VkCommandPool command_pool) {
    VkCommandBufferAllocateInfo info {
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

VkRenderPass create_render_pass(VkDevice device, VkFormat color_format) {
    VkAttachmentDescription color_attachement {
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

    /* VkAttachmentDescription depth_attachment { */
    /*     .flags = 0, */
    /*     .format = depth_format, */
    /*     .samples = VK_SAMPLE_COUNT_1_BIT, */
    /*     .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, */
    /*     .storeOp = VK_ATTACHMENT_STORE_OP_STORE, */
    /*     .stencilLoadOp= VK_ATTACHMENT_LOAD_OP_CLEAR, */
    /*     .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, */
    /*     .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, */
    /*     .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL */
    /* }; */

    std::array attachments {color_attachement};

    VkAttachmentReference attachment_reference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };


    /* VkAttachmentReference depth_attachment_refrence { */
    /*     .attachment = 1, */
    /*     .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL */
    /* }; */

    VkSubpassDescription subpass {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_reference,
        .pResolveAttachments = nullptr,
        /* .pDepthStencilAttachment = &depth_attachment_refrence, */
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };

    VkSubpassDependency color_dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    /* VkSubpassDependency depth_dependency { */
    /*     .srcSubpass = VK_SUBPASS_EXTERNAL, */
    /*     .dstSubpass = 0, */
    /*     .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, */
    /*     .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, */
    /*     .srcAccessMask = 0, */
    /*     .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, */
    /*     .dependencyFlags = 0 */
    /* }; */

    std::array dependencies {color_dependency};
    VkRenderPassCreateInfo render_pass_info {
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
    check_vk(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass));
    return render_pass;
}

constexpr VkClearValue clear_color {{{0.f, 0.f, 0.f, 1.f}}};

VkRenderPassBeginInfo render_pass_begin_info(VkRenderPass render_pass, VkFramebuffer fb, VkExtent2D extent) {
    VkRenderPassBeginInfo render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = render_pass,
        .framebuffer = fb,
        .renderArea = {
            .offset = {0, 0},
            .extent = extent
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color
    };
    return render_pass_info;
}

VkDescriptorSetLayout create_descriptor_set_layout(VkDevice device) 
{
    VkDescriptorSetLayoutBinding ubo_layout_binding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };
    

    VkDescriptorSetLayoutCreateInfo info {
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

VkDescriptorPool create_descriptor_pool(VkDevice device, uint32_t frame_count) {
    VkDescriptorPoolSize size {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = frame_count,
    };

    VkDescriptorPoolCreateInfo info {
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


template<uint32_t FRAME_COUNT>
std::array<VkDescriptorSet, FRAME_COUNT> allocate_descriptor_sets(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool) {
    std::array<VkDescriptorSetLayout, FRAME_COUNT> layouts{layout, layout};
    VkDescriptorSetAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool,
        .descriptorSetCount = FRAME_COUNT,
        .pSetLayouts = layouts.data()
    };

    std::array<VkDescriptorSet, FRAME_COUNT> descriptor_sets;
    check_vk(vkAllocateDescriptorSets(device,  &alloc_info, descriptor_sets.data()));
    return descriptor_sets;
}

void fill_descriptor_set(VkDevice device, VkDescriptorSet set, UniformBuffer const& ub) {
    VkDescriptorBufferInfo buffer_info {
        .buffer = ub.buffer.buffer,
            .offset = 0,
            .range = sizeof(UniformBufferObject)
    };

    VkWriteDescriptorSet desc_write {
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

} // namespace init

constexpr glm::vec3 red {1.f, 0.f, 0.f};
constexpr glm::vec3 blue {0.f, 0.f, 1.f};
constexpr glm::vec3 green {0.f, 1.f, 0.f};

constexpr float p {0.5f}; 
const std::vector<Vertex> verticies_back {
    {{-p, -p, -p}, red}, // 0
    {{-p, p, -p}, green}, // 1
    {{p, p, -p}, blue}, // 2
    {{p, -p, -p}, red}, // 3
    {{p, -p, p}, green}, // 4
    {{p, p, p}, blue}, // 5
    {{-p, p, p}, red}, // 6
    {{-p, -p, p}, green}, // 7
};

const std::vector<uint16_t> indices {
    0, 2, 1, 0, 3, 2, // front
    3, 2, 4, 5, 4, 2, // right
    1, 0, 6, 0, 7, 6, // left
    1, 2, 6, 2, 6, 5, // up
    0, 3, 7, 3, 7, 4, // down
    7, 4, 5, 7, 5, 6, // back
};

Engine::Engine(): 
    window_{NAME},
    instance_{init::create_instance()},
    surface_{init::create_surface(instance_, window_.handle())}
{
    init::DeviceFinder device_finder{instance_, surface_};
    phys_device_ = device_finder.find();
    device_ = device_finder.logical(phys_device_);
    auto swapchain_details = SwapChainSupportDetails::create(phys_device_, surface_);

    swapchain_ = Swapchain::create(
        swapchain_details,
        phys_device_,
        device_,
        surface_,
        window_
    );
    
    // TOOD: prevent reconstructions, make querable object on top of the VkDevice
    auto queue_families = QueueFamilyIndicies::from(phys_device_, surface_);

    graphics_queue_ = queue_families.graphics_queue(device_);
    present_queue_ = queue_families.present_queue(device_);

    vertex_shader_ = get_shader(device_, "build/shaders/triangle.vert.spv");
    fragment_shader_ = get_shader(device_, "build/shaders/triangle.frag.spv");

    descriptor_set_layout_ = init::create_descriptor_set_layout(device_);
    desc_pool_ = init::create_descriptor_pool(device_, FRAME_OVERLAP);

    frame_data_ = {};

    auto desc_sets = init::allocate_descriptor_sets<FRAME_OVERLAP>(
        device_,
        descriptor_set_layout_,
        desc_pool_ 
    );

    uint32_t idx{};
    for (auto& frame: frame_data_) {
        auto cmd_pool_ = init::create_command_pool(phys_device_, device_, surface_);
        frame.cmd_pool = cmd_pool_;
        frame.cmd_buffer = init::allocate_command_buffer(device_, cmd_pool_);
        frame.render_finished = init::create_semaphore(device_);
        frame.image_ready = init::create_semaphore(device_);
        frame.frame_ready = init::create_signaled_fence(device_);
        frame.uniform_buffer = create_uniform_buf();

        frame.descriptor_set = desc_sets.at(idx);
        init::fill_descriptor_set(device_, frame.descriptor_set, frame.uniform_buffer);
        idx++;
    }
    
    render_pass_ = init::create_render_pass(device_, swapchain_.color_format);

    extent_ = init::extent(swapchain_details.capabilities, window_.size());
    viewport_ = Viewport(extent_);

    const auto binding_desc = binding_description();
    const auto attribute_desc = attribute_descritptions();

    pipeline_ = PipelineBuilder{}
        .set_viewport(viewport_)
        .set_fragment(fragment_shader_)
        .set_vertex(vertex_shader_)
        .set_render_pass(render_pass_)
        .set_descriptors(binding_desc, std::vector(attribute_desc.begin(), attribute_desc.end()))
        .build(device_);

    // TODO: Key callback
    frame_buffers_ = create_frame_buffers(render_pass_);
    
    vertex_buffer_ = create_vertex_buf(verticies_back);
    index_buffer_ = create_index_buf(indices);
}

uint32_t find_memory_type_idx(VkPhysicalDevice device, uint32_t type_filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memory_props;
    vkGetPhysicalDeviceMemoryProperties(device, &memory_props);
    uint32_t res {0};
    for (uint32_t idx{0}; idx < memory_props.memoryTypeCount; ++idx) {
        bool prop_pattern_matches = (memory_props.memoryTypes[idx].propertyFlags & props) == props;
        bool type_matches = type_filter & (1 << idx);

        if (prop_pattern_matches and type_matches) {
            res = idx;
            break;
        }
    }
    assert(res != 0);
    return res;
}

std::vector<VkFramebuffer> Engine::create_frame_buffers(VkRenderPass renderpass) const 
{
    size_t const image_count {swapchain_.views.size()};
    std::vector<VkFramebuffer> frame_buffers;
    frame_buffers.resize(image_count);
    
    for (size_t idx{}; idx < image_count; ++idx) {
        std::array attachement {swapchain_.views[idx]};
        VkFramebufferCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderpass;
        info.attachmentCount = attachement.size();
        info.pAttachments = attachement.data();
        info.width = extent_.width;
        info.height = extent_.height;
        info.layers = 1;
        check_vk(vkCreateFramebuffer(device_, &info, nullptr, &frame_buffers[idx]));
    }
    return frame_buffers; 
}

Buffer Engine::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props) const {
    VkBufferCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Buffer buffer;
    check_vk(vkCreateBuffer(device_, &create_info, nullptr, &buffer.buffer));

    VkMemoryRequirements mem_reqs; 
    vkGetBufferMemoryRequirements(device_, buffer.buffer, &mem_reqs);
    
    VkMemoryAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = find_memory_type_idx(phys_device_, mem_reqs.memoryTypeBits, props)
    };

    check_vk(vkAllocateMemory(device_, &alloc_info, nullptr, &buffer.memory));
    vkBindBufferMemory(device_, buffer.buffer, buffer.memory, 0);
    buffer.size = size;
    return buffer;
}

UniformBuffer Engine::create_uniform_buf() const {
    constexpr size_t ubo_size {sizeof(UniformBufferObject)};
    UniformBuffer ub;
    ub.buffer = create_buffer(
        ubo_size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    vkMapMemory(device_, ub.buffer.memory, 0, ubo_size, 0, &ub.mapped);
    return ub;
}

void Engine::destroy_buffer(Buffer& buffer) const {
    vkDestroyBuffer(device_, buffer.buffer, nullptr);
    vkFreeMemory(device_, buffer.memory, nullptr);
    buffer.size = 0;
}

void Engine::copy_buffer(void const* source, Buffer& dst) const {
    void* data;
    vkMapMemory(device_, dst.memory, 0, dst.size, 0, &data);
    memcpy(data, source, dst.size);
    vkUnmapMemory(device_, dst.memory);
}

void Engine::copy_buffer(Buffer const& source, Buffer& dst) const 
{
    auto const& frame = current_frame();
    auto cmd_buffer = init::allocate_command_buffer(device_, frame.cmd_pool);

    VkCommandBufferBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd_buffer, &begin_info);
    VkBufferCopy copy {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = source.size,
    };
    
    vkCmdCopyBuffer(cmd_buffer, source.buffer, dst.buffer, 1, &copy);
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;
    
    vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(device_); // TODO: futures?

    vkFreeCommandBuffers(device_, frame.cmd_pool, 1, &cmd_buffer);
}

void Engine::record(VkCommandBuffer cmd_buff, size_t count, VkDescriptorSet desc_set) const {
    auto& frame = current_frame();

    VkCommandBufferBeginInfo buffer_begin_info {};
    buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd_buff, &buffer_begin_info);

    auto rpb_info = init::render_pass_begin_info(render_pass_, frame.framebuffer, extent_);
    vkCmdBeginRenderPass(cmd_buff, &rpb_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.pipeline);
    vkCmdSetViewport(cmd_buff, 0, 1, &viewport_.viewport);
    vkCmdSetScissor(cmd_buff, 0, 1, &viewport_.scissors);
    VkDeviceSize offset {0};
    vkCmdBindVertexBuffers(cmd_buff, 0, 1, &vertex_buffer_.buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buff, index_buffer_.buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(
        cmd_buff,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_.layout,
        0,
        1,
        &desc_set,
        0,
        nullptr
    );
    vkCmdDrawIndexed(cmd_buff, count, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd_buff);
    check_vk(vkEndCommandBuffer(cmd_buff));
}

void Engine::prepare_framedata(uint32_t framebuffer_idx) 
{
    auto& frame = current_frame();
    frame.framebuffer = frame_buffers_.at(framebuffer_idx);
    vkResetCommandBuffer(frame.cmd_buffer, 0); 
}

void Engine::run() {
    while (not window_.should_close()) {
        glfwPollEvents();
        draw();
        ++frame_number_;
    }
}

constexpr auto camera_pos = glm::vec3(0.f, 0.f, 2.f);
constexpr glm::vec3 target {0.f, 0.f, 0.f};


void Engine::draw() 
{
    auto& frame = current_frame();
    check_vk(vkWaitForFences(device_, 1, &frame.frame_ready, VK_TRUE, 1000000000));
    vkResetFences(device_, 1, &frame.frame_ready); 

    uint32_t image_index{};
    VkResult acquired_image {vkAcquireNextImageKHR(
        device_, swapchain_.swapchain, UINT64_MAX, frame.image_ready, VK_NULL_HANDLE, &image_index)};
    if (acquired_image == VK_ERROR_OUT_OF_DATE_KHR or acquired_image == VK_SUBOPTIMAL_KHR) {
        // TODO: Might not resize, handle it in the GLFW window too
        swapchain_.recreate(window_, phys_device_, device_, surface_);
        return; 
    }

    check_vk(acquired_image);
    prepare_framedata(image_index);
    frame.uniform_buffer.copy(UniformBufferObject::current(extent_, camera_pos, target));
    record(frame.cmd_buffer, indices.size(), frame.descriptor_set);
    submit(frame.cmd_buffer, image_index);
}

void Engine::submit(VkCommandBuffer cmd_buf, uint32_t image_index) const {
    auto const& frame = current_frame();

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.image_ready,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &frame.render_finished
    };
    check_vk(vkQueueSubmit(graphics_queue_, 1, &submit_info, frame.frame_ready));
    VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.render_finished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_.swapchain,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };
    check_vk(vkQueuePresentKHR(present_queue_, &present_info));
}

Buffer Engine::create_vertex_buf(std::span<Vertex const> verticies) const 
{
    size_t const buffer_size {verticies.size() * sizeof(verticies[0])};
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

Buffer Engine::create_index_buf(std::span<uint16_t const> indicies) const {
    size_t const buffer_size {indicies.size() * sizeof(indicies[0])};
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
    for (const auto& deleter : deletion_queue_ | std::views::reverse) 
    {
        deleter();
    }
    glfwTerminate();
}

