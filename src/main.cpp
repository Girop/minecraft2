#include <assert.h>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <fmt/format.h>
#include <fstream>
#include <array>
#include <optional>
#include <algorithm>
#include <vector>
#include <set>
#include <filesystem>
#include <glm/glm.hpp>


constexpr uint32_t FRAME_COUNT {2};

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
    
    VkExtent2D choose_swap_extent(GLFWwindow* window) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D extent {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height)
        };

        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return extent;
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


VkPhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface) {
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
    GLFWwindow* window,
    VkPhysicalDevice phys_device,
    VkDevice device,
    VkSurfaceKHR surface
) {
    auto format = details.choose_format();
    auto extent = details.choose_swap_extent(window);
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

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;


    static VkVertexInputBindingDescription binding_description() {
        return VkVertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    static std::array<VkVertexInputAttributeDescription, 2> attribute_descritptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributes{};
        auto& position {attributes.at(0)};
        position.binding = 0;
        position.location = 0;
        position.format = VK_FORMAT_R32G32_SFLOAT;
        position.offset = offsetof(Vertex, pos);


        auto& clr {attributes.at(1)};
        clr.binding = 0;
        clr.location = 1;
        clr.offset = offsetof(Vertex, color);
        clr.format = VK_FORMAT_R32G32B32_SFLOAT;

        return attributes;
    }
};


std::vector<char> load_shader(std::filesystem::path const& path) {
    std::ifstream fstream {path, std::ios::ate | std::ios::binary};

    if (!fstream.is_open()) {
        fmt::println("Failed to open: {}", path.string());
        abort();
    }

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

    auto binding_desc = Vertex::binding_description();
    auto attribute_desc = Vertex::attribute_descritptions();

    VkPipelineVertexInputStateCreateInfo vertex_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_desc,
        .vertexAttributeDescriptionCount = attribute_desc.size(),
        .pVertexAttributeDescriptions = attribute_desc.data()
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

std::vector<VkCommandBuffer> allocate_command_buffers(VkDevice device, VkCommandPool command_pool) {
    VkCommandBufferAllocateInfo info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 2,
    };

    std::vector<VkCommandBuffer> buffer;
    buffer.resize(FRAME_COUNT);
    assert(vkAllocateCommandBuffers(device, &info, buffer.data()) == VK_SUCCESS);
    return buffer;
}

void record_command_buffer(
    VkCommandBuffer buffer,
    VkFramebuffer framebuffer,
    VkRenderPass render_pass,
    VkExtent2D extent,
    VkPipeline pipeline,
    VkBuffer vert_buf,
    std::vector<Vertex> const& verticies
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

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(buffer, 0, 1, &vert_buf, offsets);

    vkCmdDraw(buffer, verticies.size(), 1, 0, 0);

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

struct SynchronizationPrimitives {
    std::vector<VkSemaphore> image_available;
    std::vector<VkSemaphore> rendering_finished;
    std::vector<VkFence> inlight_fence;

    static SynchronizationPrimitives create(VkDevice device, uint32_t count) {
        SynchronizationPrimitives primitives;
        primitives.image_available.resize(count);
        primitives.rendering_finished.resize(count);
        primitives.inlight_fence.resize(count);


        for (uint32_t idx{}; idx < count; ++idx) {
            primitives.inlight_fence.at(idx) = create_signaled_fence(device);
            primitives.image_available.at(idx) = create_semaphore(device);
            primitives.rendering_finished.at(idx) = create_semaphore(device);
        }
        return primitives;
    }

    void destroy(VkDevice referenced_device) {
        for (uint32_t idx {0}; idx < image_available.size(); ++idx) {
            vkDestroyFence(referenced_device, inlight_fence.at(idx), nullptr);
            vkDestroySemaphore(referenced_device, image_available.at(idx), nullptr);
            vkDestroySemaphore(referenced_device, rendering_finished.at(idx), nullptr);
        }
    }
};

struct SwapchainFixedData {
    SwapChainResult swapchain;
    std::vector<VkImageView> image_views;
    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers; 

    static SwapchainFixedData create(VkPhysicalDevice phys_device, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window)
    {
        auto details = SwapChainSupportDetails::query_from(phys_device, surface);
        auto swapchain = create_swapchain(details, window, phys_device, device, surface);
        auto image_views = create_imageviews(swapchain, device);
        auto render_pass = create_render_pass(device, swapchain.used_format);
        auto framebuffers = get_framebuffers(device, image_views, render_pass, swapchain.extent);

        return SwapchainFixedData {
            .swapchain = swapchain,
            .image_views = image_views,
            .render_pass = render_pass,
            .framebuffers = framebuffers,
        };
    }

    void recreate(GLFWwindow* handle, VkPhysicalDevice phys_device, VkDevice device, VkSurfaceKHR surface)
    {
        vkDeviceWaitIdle(device);
        destroy(device);

        int width, height;
        glfwGetFramebufferSize(handle, &width, &height);

        while (width == 0 or height == 0) {
            glfwGetFramebufferSize(handle, &width, &height);
            glfwWaitEvents();
        }
        *this = SwapchainFixedData::create(phys_device, device, surface, handle);
    }

    void destroy(VkDevice device) 
    {
        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        vkDestroyRenderPass(device, render_pass, nullptr);
        for (auto view : image_views) {
            vkDestroyImageView(device, view, nullptr);
        }
        vkDestroySwapchainKHR(device, swapchain.handle, nullptr);
    }
};

void draw_frame(
    GLFWwindow* window,
    VkDevice device,
    VkPhysicalDevice phys_device,
    VkSurfaceKHR surface,
    SwapchainFixedData& swapchain,
    uint32_t current_frame,
    SynchronizationPrimitives const& primitives,
    VkCommandBuffer command_buffer,
    VkPipeline pipeline,
    VkQueue graphics_queue,
    VkQueue present_queue,
    VkBuffer vert_buffer,
    std::vector<Vertex> const& verticies
) {
    auto current_fence = primitives.inlight_fence.at(current_frame);
    auto image_avail = primitives.image_available.at(current_frame);
    auto render_finished = primitives.rendering_finished.at(current_frame);

    vkWaitForFences(device, 1, &current_fence, VK_TRUE,  UINT64_MAX);
    
    uint32_t image_index;
    VkResult success = vkAcquireNextImageKHR(device, swapchain.swapchain.handle, UINT64_MAX, image_avail, VK_NULL_HANDLE, &image_index);
    if (success == VK_ERROR_OUT_OF_DATE_KHR or success == VK_SUBOPTIMAL_KHR) {
        swapchain.recreate(window, phys_device, device, surface);
        return; 
    } 
    assert(success == VK_SUCCESS);
    vkResetFences(device, 1, &current_fence);

    vkResetCommandBuffer(command_buffer, 0);
    record_command_buffer(
        command_buffer,
        swapchain.framebuffers.at(image_index),
        swapchain.render_pass,
        swapchain.swapchain.extent,
        pipeline,
        vert_buffer,
        verticies
    );

    
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

    assert(vkQueueSubmit(graphics_queue, 1, &info, current_fence) == VK_SUCCESS);

    VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain.swapchain.handle,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };

    vkQueuePresentKHR(present_queue, &present_info);
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

std::pair<VkBuffer, VkDeviceMemory> create_buffer(
    VkPhysicalDevice phys_device,
    VkDevice device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags props)
{
    VkBufferCreateInfo buff_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };


    std::pair<VkBuffer, VkDeviceMemory> result;
    assert(vkCreateBuffer(device, &buff_info, nullptr, &result.first) == VK_SUCCESS);

    VkMemoryRequirements mem_reqs; 
    vkGetBufferMemoryRequirements(device, result.first, &mem_reqs);
    
    VkMemoryAllocateInfo allocation_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr, 
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = find_memory_type_idx(phys_device, mem_reqs.memoryTypeBits, props)
    };

    assert(vkAllocateMemory(device, &allocation_info, nullptr, &result.second) == VK_SUCCESS);
    vkBindBufferMemory(device, result.first, result.second, 0);
    return result;
}


// TODO: should be - void copy_buffer(VkBuffer source, VkBuffer destination, VkDeviceSize size)
void copy_buffer(
    VkBuffer source,
    VkBuffer destination,
    VkDeviceSize size,
    VkDevice device,
    VkCommandPool cmdpool,
    VkQueue graphics_queue
) {
    VkCommandBufferAllocateInfo allocate_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = cmdpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd_buffer; 
    vkAllocateCommandBuffers(device, &allocate_info, &cmd_buffer);
    

    VkCommandBufferBeginInfo begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0
    };

    vkBeginCommandBuffer(cmd_buffer, &begin_info);
    VkBufferCopy copy {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };
    
    vkCmdCopyBuffer(cmd_buffer, source, destination, 1, &copy);
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr, 
    };
    
    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkDeviceWaitIdle(device);

    vkFreeCommandBuffers(device, cmdpool, 1, &cmd_buffer);
}


std::pair<VkBuffer, VkDeviceMemory> create_vertex_buffer(
    VkPhysicalDevice phys_device,
    VkDevice device,
    VkCommandPool cmdpool,
    VkQueue graphics_queue,
    std::vector<Vertex> const& verticies
) {
    size_t mem_size = sizeof(verticies.at(0)) * verticies.size();

    uint32_t staging_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
     
    auto [staging_buf, staging_mem] = create_buffer(
        phys_device,
        device,
        mem_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        staging_prop_flags
    );

    void* data;
    vkMapMemory(device, staging_mem, 0, mem_size, 0, &data);
    memcpy(data, verticies.data(), mem_size);
    vkUnmapMemory(device, staging_mem);

    uint32_t usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    auto [vertex_buf, vertex_mem] = create_buffer(phys_device, device, mem_size, usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(staging_buf, vertex_buf, mem_size, device, cmdpool, graphics_queue);
    vkDestroyBuffer(device, staging_buf, nullptr);
    vkFreeMemory(device, staging_mem, nullptr);

    return {vertex_buf, vertex_mem};
}



int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) 
{ 

    const std::vector<Vertex> verticies {
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},

        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    };

    const std::vector<uint16_t> indices {
        0, 1, 2, //
        2, 3, 0
    };

    fmt::println("Starting");
    auto window = init_glfw();
    VkInstance instance {create_instance()};
    VkSurfaceKHR surface {create_surface(window, instance)};
    VkPhysicalDevice phys_device {find_physical_device(instance, surface)};
    VkDevice device {create_logical_device(phys_device, surface)};

    VkPipelineLayout pipeline_layout {create_pipeline_layout(device)};

    auto vert_module = create_shader_module(device, load_shader("shaders/vert.spv"));
    auto frag_module = create_shader_module(device, load_shader("shaders/frag.spv"));

    auto swapchain_data = SwapchainFixedData::create(phys_device, device, surface, window);

    auto pipeline = create_pipeline(
        device,
        swapchain_data.swapchain.extent,
        swapchain_data.render_pass,
        pipeline_layout,
        vert_module,
        frag_module
    );

    auto command_pool = create_command_pool(phys_device, device, surface);
    auto command_buffer = allocate_command_buffers(device, command_pool);

    auto graph_queue = get_graph_queue(device, phys_device, surface);
    auto present_queue = get_present_queue(device, phys_device, surface);
    auto primitives = SynchronizationPrimitives::create(device, FRAME_COUNT);
    auto [vertex_buf, vertex_mem] = create_vertex_buffer(
        phys_device,
        device,
        command_pool,
        graph_queue,
        verticies
    );

    uint32_t current_frame {0};
    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        draw_frame(
            window,
            device,
            phys_device,
            surface,
            swapchain_data,
            current_frame,
            primitives,
            command_buffer.at(current_frame),
            pipeline,
            graph_queue,
            present_queue,
            vertex_buf,
            verticies
        );
        current_frame = (current_frame + 1) % FRAME_COUNT;
    }

    vkDeviceWaitIdle(device);

    vkDestroyBuffer(device, vertex_buf, nullptr);
    vkFreeMemory(device, vertex_mem, nullptr);

    primitives.destroy(device);
    vkDestroyCommandPool(device, command_pool, nullptr);
    swapchain_data.destroy(device);
    vkDestroyShaderModule(device, vert_module, nullptr);
    vkDestroyShaderModule(device, frag_module, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    fmt::print("Success\n");
    return 0;
} 
