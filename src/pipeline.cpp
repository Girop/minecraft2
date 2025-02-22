#include <array>
#include <span>
#include "pipeline.hpp"
#include "utils/vulkan.hpp"


VkPipelineLayout create_pipeline_layout(VkDevice device, std::span<VkDescriptorSetLayout const> layouts) {
    VkPipelineLayoutCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = layouts.size();
    info.pSetLayouts = layouts.data();
    VkPipelineLayout layout;
    utils::check_vk(vkCreatePipelineLayout(device, &info, nullptr, &layout));
    return layout;
}

constexpr std::array states {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

VkPipelineDynamicStateCreateInfo PipelineBuilder::dynamic_state_info() const { 
    VkPipelineDynamicStateCreateInfo state_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = states.size(),
        .pDynamicStates = states.data()
    };
    return state_info;
}

VkPipelineColorBlendStateCreateInfo PipelineBuilder::blend_info() const {
    VkPipelineColorBlendStateCreateInfo blend_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &attachment_,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };
    return blend_info;
}

PipelineBuilder::PipelineBuilder() {
    // TODO: lifetime issues, unnecessary ctr
    attachment_ = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
   };
}

PipelineBuilder& PipelineBuilder::set_descriptors(
        VkVertexInputBindingDescription binding,
        Vertex::AttributeDescriptions attribute
) {
    inpute_vertex_.binding_desriptions = binding;
    inpute_vertex_.attribute_descriptions = attribute;
    
    size_t const attribute_count {sizeof attribute / sizeof attribute.color};
    inpute_vertex_.info = VkPipelineVertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &inpute_vertex_.binding_desriptions,
        .vertexAttributeDescriptionCount = attribute_count,
        .pVertexAttributeDescriptions = reinterpret_cast<VkVertexInputAttributeDescription*>(&inpute_vertex_.attribute_descriptions)
    };
    return *this;
}

PipelineBuilder& PipelineBuilder::set_descriptor_sets(VkDescriptorSetLayout ubo, VkDescriptorSetLayout texture) {
    descriptor_layouts_ = std::vector{ubo, texture};
    return *this;
}

Pipeline PipelineBuilder::build(VkDevice device) const { 
    auto const ass_info = assembly_info();
    auto const rasterizer = rasterizer_info();
    auto const multisampling = multisampling_info();
    auto const dynamic_states = dynamic_state_info();
    auto const blend = blend_info();
    auto const pipeline_layout = create_pipeline_layout(device, descriptor_layouts_);

    VkGraphicsPipelineCreateInfo pipeline_info {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = shader_stages_.size();
    pipeline_info.pStages = shader_stages_.data();
    pipeline_info.pVertexInputState = &inpute_vertex_.info;
    pipeline_info.pInputAssemblyState = &ass_info;
    pipeline_info.pViewportState = &viewport_.info;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDynamicState = &dynamic_states;
    pipeline_info.pDepthStencilState = &depth_create_info_;
    pipeline_info.pColorBlendState = &blend;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline pipeline;
    utils::check_vk(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));
    return Pipeline{pipeline, pipeline_layout};
}


VkPipelineMultisampleStateCreateInfo PipelineBuilder::multisampling_info() const {
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
    return multisampling_info;
}

VkPipelineInputAssemblyStateCreateInfo PipelineBuilder::assembly_info() const {
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE, 
    };
    return input_assembly_info;
}

PipelineBuilder& PipelineBuilder::set_viewport(Viewport const& viewport) {
    viewport_.viewport = viewport;
    viewport_.info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport_.viewport.viewport,
        .scissorCount = 1,
        .pScissors = &viewport_.viewport.scissors,
    };
    return *this;
}


constexpr auto SHADER_ENTRY_NAME {"main"};

PipelineBuilder& PipelineBuilder::set_vertex(VkShaderModule vertex) {
    VkPipelineShaderStageCreateInfo info_vert {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex,
        .pName = SHADER_ENTRY_NAME,
        .pSpecializationInfo = nullptr
    }; 
    vertex_ = vertex;   
    shader_stages_.push_back(info_vert);
    return *this;
}

PipelineBuilder& PipelineBuilder::set_fragment(VkShaderModule fragment) {
    VkPipelineShaderStageCreateInfo info_frag {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment, 
        .pName = SHADER_ENTRY_NAME,
        .pSpecializationInfo = nullptr
    };
    fragment_ = fragment; 
    shader_stages_.push_back(info_frag);
    return *this;
}


VkPipelineRasterizationStateCreateInfo PipelineBuilder::rasterizer_info() const {
    VkPipelineRasterizationStateCreateInfo rasterizer_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };
    return rasterizer_info;
}

