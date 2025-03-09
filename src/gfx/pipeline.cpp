#include <array>
#include <span>
#include "pipeline.hpp"
#include "utils.hpp"

namespace 
{

VkPipelineInputAssemblyStateCreateInfo assembly_info() {
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE, 
    };
    return input_assembly_info;
}

VkPipelineRasterizationStateCreateInfo rasterizer_info() {
    VkPipelineRasterizationStateCreateInfo rasterizer_info 
    {
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

VkPipelineMultisampleStateCreateInfo multisampling_info() {
    VkPipelineMultisampleStateCreateInfo multisampling_info 
    {
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

constexpr std::array states {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

} // namespace 

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

VkPipelineLayout PipelineBuilder::create_pipeline_layout(VkDevice const device) const
{
    VkPipelineLayoutCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = descriptor_layouts_.size();
    info.pSetLayouts = descriptor_layouts_.data();
    VkPipelineLayout layout;
    utils::check_vk(vkCreatePipelineLayout(device, &info, nullptr, &layout));
    return layout;
}

VkPipelineColorBlendStateCreateInfo PipelineBuilder::blend_info() const {
    VkPipelineColorBlendStateCreateInfo blend_info 
    {
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

PipelineBuilder PipelineBuilder::set_descriptions(Descriptions const& descriptions) {
    descriptions_ = descriptions;
    
    vertex_info_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &descriptions_.binding,
        .vertexAttributeDescriptionCount = Vertex::COUNT,
        .pVertexAttributeDescriptions = reinterpret_cast<VkVertexInputAttributeDescription*>(&descriptions_.attribute)
    };
    return *this;
}

PipelineBuilder PipelineBuilder::set_descriptor_sets(VkDescriptorSetLayout ubo, VkDescriptorSetLayout texture) {
    descriptor_layouts_ = std::vector{ubo, texture};
    return *this;
}


PipelineBuilder PipelineBuilder::set_render_pass(VkRenderPass render_pass) 
{
    render_pass_ = render_pass; 
    return *this;
}

PipelineBuilder PipelineBuilder::set_depth_testing(VkPipelineDepthStencilStateCreateInfo depthstencil) 
{
    depth_create_info_ = depthstencil;
    return *this;
}

Pipeline PipelineBuilder::build(VkDevice device) const { 
    auto const ass_info = assembly_info();
    auto const rasterizer = rasterizer_info();
    auto const multisampling = multisampling_info();
    auto const dynamic_states = dynamic_state_info();
    auto const blend = blend_info();
    auto const pipeline_layout = create_pipeline_layout(device);

    VkGraphicsPipelineCreateInfo pipeline_info {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = shader_stages_.size();
    pipeline_info.pStages = shader_stages_.data();
    pipeline_info.pVertexInputState = &vertex_info_;
    pipeline_info.pInputAssemblyState = &ass_info;
    pipeline_info.pViewportState = &viewport_;
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



PipelineBuilder PipelineBuilder::set_viewport(Viewport const& viewport) 
{
    viewport_ = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport.viewport,
        .scissorCount = 1,
        .pScissors = &viewport.scissors,
    };
    return *this;
}

VkPipelineShaderStageCreateInfo PipelineBuilder::shader_info(Shader const& shader) const 
{
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = shader.type == ShaderType::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shader.module,
        .pName = SHADER_MAIN,
        .pSpecializationInfo = nullptr
    };
}

PipelineBuilder PipelineBuilder::set_shader(Shader const& shader) 
{
    shader_stages_.emplace_back(shader_info(shader));
    switch (shader.type) {
        case ShaderType::Vertex:
            vertex_ = shader.module;
            break;
        case ShaderType::Fragment:
            fragment_ = shader.module;
            break;
    }
    return *this;
}

