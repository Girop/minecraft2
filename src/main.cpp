#include <assert.h>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <fmt/format.h>
#include <fstream>
#include <array>
#include <optional>
#include <vector>
#include <set>
#include <filesystem>


VkInstance create_instance() {
    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    std::array extensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    std::array activated_validation_layers {
        "VK_LAYER_KHRONOS_validation"
    };

    uint32_t layerCount{};
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available_layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available_layers.data());
    
    for (auto& layer: activated_validation_layers) {
        auto found_it = std::ranges::find_if(available_layers, [&layer](auto& elem) {
            return std::strcmp(elem.layerName, layer) == 0;
        });
        assert(found_it != available_layers.end());
    }

    VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = activated_validation_layers.size(),
        .ppEnabledLayerNames = activated_validation_layers.data(),
        .enabledExtensionCount = extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkInstance instance;
    assert(vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS);
    return instance;
}


struct QueueFamilyIndicies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    static QueueFamilyIndicies from(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndicies indicies;

        uint32_t queue_famliy_count{};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_famliy_count, nullptr);
        assert(queue_famliy_count != 0);
        std::vector<VkQueueFamilyProperties> queue_families(queue_famliy_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_famliy_count, queue_families.data());

        int queue_idx{};
        for (auto const& queue: queue_families) {
            if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indicies.graphics = queue_idx;
            }

            VkBool32 supports_present {false};
            vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_idx, surface, &supports_present);
            if (supports_present) {
                indicies.present = queue_idx;
            }

            queue_idx++;
        }
        return indicies;
    }


    bool complete() const {
        return graphics.has_value() and present.has_value();
    }
};


constexpr std::array device_extensions {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};


bool supports_extensions(VkPhysicalDevice device) {
    uint32_t extension_count{};

    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : extensions) {
        required_extensions.erase(extension.extensionName);
    }
     
    return required_extensions.empty();
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;

    static SwapChainSupportDetails query_from(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t format_count {};
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

        if (format_count != 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
        } 

        uint32_t presentation_modes_count{};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_modes_count, nullptr);
        if (presentation_modes_count != 0) {
            details.present_modes.resize(presentation_modes_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_modes_count, details.present_modes.data());
        }
        return details;
    }
    
    VkExtent2D choose_swap_extent() const {
        return capabilities.currentExtent; // FIXME: HERE MAY LAY A PROBLEM WITH COORDINATES
    }

    VkSurfaceFormatKHR choose_format() const {
        for (auto const& format : formats) {
            if (format.format == VK_FORMAT_R8G8B8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return formats.front();
    }

    VkPresentModeKHR choose_present_mode() const {
        for (auto const& mode : present_modes) {
           if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
               return mode;
           }
        }
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    uint32_t image_count() const {
        uint32_t image_count {capabilities.minImageCount + 1};
        if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
            image_count = capabilities.maxImageCount;
        }
        return image_count;
        
    }


    bool supported() const {
        return !formats.empty() and !present_modes.empty();
    }
};


bool is_typical_gpu(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties device_props;
    VkPhysicalDeviceFeatures dev_features;

    vkGetPhysicalDeviceProperties(device, &device_props);
    vkGetPhysicalDeviceFeatures(device, &dev_features);

    return device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU and dev_features.geometryShader;
}


bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    bool extensisons_supported {supports_extensions(device)};
    bool matching_queues_present {QueueFamilyIndicies::from(device, surface).complete()};
    bool swapchain_complete {
        extensisons_supported 
            ? SwapChainSupportDetails::query_from(device, surface).supported() 
            : false
    };

    return is_typical_gpu(device)
        and extensisons_supported
        and swapchain_complete
        and matching_queues_present;
}


VkPhysicalDevice create_phy_device(VkInstance instance, VkSurfaceKHR surface) {
    VkPhysicalDevice chosen_device {VK_NULL_HANDLE};
    
    uint32_t device_count{0};
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    assert(device_count != 0);
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    for (auto const& device : devices) {
        if (is_device_suitable(device, surface)) {
            chosen_device = device;
            break;
        }
    }
    assert(chosen_device != VK_NULL_HANDLE);
    return chosen_device;
}


VkDevice create_logical_device(VkPhysicalDevice device, VkSurfaceKHR surface) {
    auto indicies = QueueFamilyIndicies::from(device, surface);
    std::set families {indicies.graphics.value(), indicies.present.value()};
    std::vector<VkDeviceQueueCreateInfo> create_infos;
    float queue_prio {1.0f};

    for (auto queue_family : families) {
        create_infos.emplace_back(
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, queue_family, 1, &queue_prio);
    }

    VkPhysicalDeviceFeatures dev_features{};

    VkDeviceCreateInfo device_create_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = static_cast<uint32_t>(create_infos.size()),
        .pQueueCreateInfos = create_infos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = device_extensions.size(),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &dev_features,
    };

    VkDevice logical_device{};
    assert(vkCreateDevice(device, &device_create_info, nullptr, &logical_device) == VK_SUCCESS);
    assert(logical_device != VK_NULL_HANDLE);
    return logical_device;
}


VkQueue get_graph_queue(VkDevice device, VkPhysicalDevice phys_device, VkSurfaceKHR surface) {
    auto queue_families {QueueFamilyIndicies::from(phys_device, surface)};
    VkQueue queue;
    vkGetDeviceQueue(device, queue_families.graphics.value(), 0, &queue);
    assert(queue != VK_NULL_HANDLE);
    return queue;
}


VkQueue get_present_queue(VkDevice device, VkPhysicalDevice phys_device, VkSurfaceKHR surface) {
    auto queue_families {QueueFamilyIndicies::from(phys_device, surface)};
    VkQueue queue;
    vkGetDeviceQueue(device, queue_families.present.value(), 0, &queue);
    assert(queue != VK_NULL_HANDLE);
    return queue;
}


VkSurfaceKHR create_surface(GLFWwindow* window, VkInstance vkinstance) {
    VkSurfaceKHR surface;
    assert(glfwCreateWindowSurface(vkinstance, window, nullptr, &surface) == VK_SUCCESS);
    return surface;
}


struct SwapChainResult {
    VkSwapchainKHR handle;
    VkFormat used_format;
    VkExtent2D extent;
    std::vector<VkImage> images;
};

std::vector<VkImage> swapchain_images(VkSwapchainKHR swapchain, VkDevice device) {
    uint32_t image_count {};
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());
    return images;
}


SwapChainResult create_swapchain(
    SwapChainSupportDetails const& details,
    VkPhysicalDevice phys_device,
    VkDevice device,
    VkSurfaceKHR surface
) {
    auto format = details.choose_format();
    auto extent = details.choose_swap_extent();
    // If queues are separate use VK_SHARING_MODE_EXCLUSIVE
    auto indicies = QueueFamilyIndicies::from(phys_device, surface);
    std::array queue_indicies {indicies.graphics.value(), indicies.present.value()};

    VkSwapchainCreateInfoKHR create_info {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
        .minImageCount = details.image_count(),
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = queue_indicies.size(),
        .pQueueFamilyIndices = queue_indicies.data(),
        .preTransform = details.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = details.choose_present_mode(),
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    VkSwapchainKHR swapchain;
    assert(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) == VK_SUCCESS);
    return {
        swapchain,
        format.format,
        extent,
        swapchain_images(swapchain, device)
    };
}


std::vector<VkImageView> create_imageviews(SwapChainResult const& swapchain, VkDevice device) {
    std::vector<VkImageView> views(swapchain.images.size());

    for (size_t idx{}; idx < swapchain.images.size(); ++idx) {
        VkImageViewCreateInfo info {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = swapchain.images[idx],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain.used_format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        assert(vkCreateImageView(device, &info, nullptr, &views[idx]) == VK_SUCCESS);
    }
    return views;
}


std::vector<char> load_shader(std::filesystem::path const& path) {
    std::ifstream fstream {path, std::ios::ate | std::ios::binary};
    assert(fstream.is_open()); 

    std::vector<char> bytes(fstream.tellg());
    fstream.seekg(0);

    fstream.read(bytes.data(), bytes.size());
    return bytes;
}


VkShaderModule create_shader_module(VkDevice device, std::vector<char> const& code) {
    VkShaderModuleCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    VkShaderModule module;
    assert(vkCreateShaderModule(device, &info, nullptr, &module) == VK_SUCCESS);
    return module;
}

VkRenderPass create_render_pass(VkDevice device, VkFormat format) {
    VkAttachmentDescription color_attachement {
        .flags = 0, 
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    
    VkAttachmentReference attachment_reference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_reference,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };

    VkSubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    VkRenderPassCreateInfo render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &color_attachement,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkRenderPass render_pass;
    assert(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) == VK_SUCCESS);
    return render_pass;
}


VkPipeline create_pipeline(
    VkDevice device,
    VkExtent2D extent,
    VkRenderPass render_pass,
    VkPipelineLayout pipeline_layout,
    VkShaderModule vert_shader,
    VkShaderModule frag_shader
) {

    VkPipelineShaderStageCreateInfo info_vert {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };


    VkPipelineShaderStageCreateInfo info_frag {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader, 
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

    std::array dynamic_states {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    std::array shaderStages {info_vert, info_frag};

    VkPipelineDynamicStateCreateInfo state_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = dynamic_states.size(),
        .pDynamicStates = dynamic_states.data()
    };

    VkPipelineVertexInputStateCreateInfo vertex_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };
 
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE, 
    };

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0,
        .maxDepth = 1.0f
    };

    VkRect2D scissors {
        .offset = {0, 0},
        .extent = extent,
    };
     

    VkPipelineViewportStateCreateInfo viewport_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissors,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    
   VkPipelineColorBlendAttachmentState attachment_info {
       .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
   };


    VkPipelineColorBlendStateCreateInfo blend_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &attachment_info,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };



    VkGraphicsPipelineCreateInfo pipeline_info {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = shaderStages.size(),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertex_info,
        .pInputAssemblyState = &input_assembly_info,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_info,
        .pRasterizationState = &rasterizer_info,
        .pMultisampleState = &multisampling_info,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &blend_info,
        .pDynamicState = &state_info,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VkPipeline pipeline;
    assert(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) == VK_SUCCESS);
    return pipeline;
}

VkPipelineLayout create_pipeline_layout(VkDevice device) {
    VkPipelineLayoutCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    VkPipelineLayout layout;
    assert(vkCreatePipelineLayout(device, &info, nullptr, &layout) == VK_SUCCESS);
    return layout;
}

GLFWwindow* init_glfw() {
    assert(glfwInit());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    return window;
}

std::vector<VkFramebuffer> get_framebuffers(
    VkDevice device,
    std::vector<VkImageView> const& imageviews,
    VkRenderPass renderpass,
    VkExtent2D extent
) {
    std::vector<VkFramebuffer> swapchain_framebuffers;
    swapchain_framebuffers.resize(imageviews.size());
    
    for (size_t idx{}; idx < imageviews.size(); ++idx) 
    {
        VkImageView attachments[] = { imageviews[idx] };
        VkFramebufferCreateInfo framebuffer_info {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = renderpass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = extent.width,
            .height = extent.height,
            .layers = 1
        };
        assert(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swapchain_framebuffers.at(idx)) == VK_SUCCESS);
    }
    return swapchain_framebuffers;
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
    assert(vkCreateCommandPool(device, &info, nullptr, &comand_pool) == VK_SUCCESS);
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
    assert(vkAllocateCommandBuffers(device, &info, &buffer) == VK_SUCCESS);
    return buffer;
}

void record_command_buffer(
    VkCommandBuffer buffer,
    VkFramebuffer framebuffer,
    VkRenderPass render_pass,
    VkExtent2D extent,
    VkPipeline pipeline
)
{
    VkCommandBufferBeginInfo buffer_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    assert(vkBeginCommandBuffer(buffer, &buffer_begin_info) == VK_SUCCESS);
    
    VkClearValue clear_color {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // {{lol}}

    VkRenderPassBeginInfo render_pass_begin {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = extent
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color
    };

    vkCmdBeginRenderPass(buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);


    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissors {
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetScissor(buffer, 0, 1, &scissors);

    vkCmdDraw(buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(buffer);

    assert(vkEndCommandBuffer(buffer) == VK_SUCCESS);
}

VkSemaphore create_semaphore(VkDevice device) {
    VkSemaphoreCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VkSemaphore sem;
    assert(vkCreateSemaphore(device, &info, nullptr, &sem) == VK_SUCCESS);
    return sem;
}

VkFence create_signaled_fence(VkDevice device) {
    VkFenceCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    VkFence fence;
    assert(vkCreateFence(device, &info, nullptr, &fence) == VK_SUCCESS);
    return fence;
}


void draw_frame(
    VkDevice device,
    VkFence inlight,
    SwapChainResult& swapchain,
    VkSemaphore image_avail,
    VkSemaphore render_finished,
    VkCommandBuffer command_buffer,
    std::vector<VkFramebuffer>& framebuffer,
    VkRenderPass render_pass,
    VkPipeline pipeline,
    VkQueue graphics_queue,
    VkQueue present_queue
) {
    vkWaitForFences(device, 1, &inlight, VK_TRUE,  UINT64_MAX);
    vkResetFences(device, 1, &inlight);

    uint32_t image_index;
    vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, image_avail, VK_NULL_HANDLE, &image_index);
    vkResetCommandBuffer(command_buffer, 0);
    record_command_buffer(command_buffer, framebuffer.at(image_index), render_pass, swapchain.extent, pipeline);

    
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_avail,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_finished
    };

    assert(vkQueueSubmit(graphics_queue, 1, &info, inlight) == VK_SUCCESS);

    VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain.handle,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };

    vkQueuePresentKHR(present_queue, &present_info);
}


int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) 
{ 
    fmt::println("Starting");
    auto window = init_glfw();
    VkInstance instance {create_instance()};
    VkSurfaceKHR surface {create_surface(window, instance)};
    VkPhysicalDevice phys_device {create_phy_device(instance, surface)};
    VkDevice device {create_logical_device(phys_device, surface)};
    auto details = SwapChainSupportDetails::query_from(phys_device, surface);
    auto swapchain {create_swapchain(details, phys_device, device, surface)};
    auto image_views = create_imageviews(swapchain, device);
    VkPipelineLayout pipeline_layout {create_pipeline_layout(device)};
    auto render_pass = create_render_pass(device, swapchain.used_format);

    auto vert_code = load_shader("shaders/vert.spv");
    auto frag_code = load_shader("shaders/frag.spv");
    auto vert_module = create_shader_module(device, vert_code);
    auto frag_module = create_shader_module(device, frag_code);

    auto pipeline = create_pipeline(device, swapchain.extent, render_pass, pipeline_layout, vert_module, frag_module);
    auto framebuffers = get_framebuffers(device, image_views, render_pass, swapchain.extent);
    auto command_pool = create_command_pool(phys_device, device, surface);

    auto command_buffer = allocate_command_buffer(device, command_pool);

    VkSemaphore image_available = create_semaphore(device);
    VkSemaphore render_finished = create_semaphore(device);
    VkFence inlight_fence = create_signaled_fence(device); 
    auto graph_queue = get_graph_queue(device, phys_device, surface);
    auto present_queue = get_present_queue(device, phys_device, surface);

    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        draw_frame(
            device,
            inlight_fence,
            swapchain,
            image_available,
            render_finished,
            command_buffer,
            framebuffers,
            render_pass,
            pipeline,
            graph_queue,
            present_queue
        );
    }

    vkDeviceWaitIdle(device);

    vkDestroyFence(device, inlight_fence, nullptr);
    vkDestroySemaphore(device, image_available, nullptr);
    vkDestroySemaphore(device, render_finished, nullptr);

    vkDestroyCommandPool(device, command_pool, nullptr);
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyShaderModule(device, vert_module, nullptr);
    vkDestroyShaderModule(device, frag_module, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    for (auto view : image_views) {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain.handle, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    fmt::print("Success\n");

    return 0;
} 
