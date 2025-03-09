#pragma once
#include <vector>
#include "viewport.hpp"
#include "vertex.hpp"
#include "shader.hpp"

struct Pipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};


class PipelineBuilder {
public:
    PipelineBuilder();
    Pipeline build(VkDevice device) const;

    PipelineBuilder set_shader(Shader const& shader);
    PipelineBuilder set_descriptor_sets(VkDescriptorSetLayout ubo, VkDescriptorSetLayout texture);
    PipelineBuilder set_viewport(Viewport const& viewport);
    PipelineBuilder set_render_pass(VkRenderPass render_pass);
    PipelineBuilder set_descriptions(Descriptions const& descriptors);
    PipelineBuilder set_depth_testing(VkPipelineDepthStencilStateCreateInfo depthstencil);
private:
    static constexpr auto SHADER_MAIN {"main"};

    VkPipelineLayout create_pipeline_layout(VkDevice const device) const;


    VkPipelineDynamicStateCreateInfo dynamic_state_info() const;
    VkPipelineColorBlendStateCreateInfo blend_info() const;
    VkPipelineShaderStageCreateInfo shader_info(Shader const& shader) const;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_;
    VkShaderModule vertex_;
    VkShaderModule fragment_;
    VkPipelineViewportStateCreateInfo viewport_;
    VkPipelineColorBlendAttachmentState attachment_;
    VkRenderPass render_pass_;
    VkPipelineDepthStencilStateCreateInfo depth_create_info_;
    VkPipelineVertexInputStateCreateInfo vertex_info_;
    Descriptions descriptions_;
    std::vector<VkDescriptorSetLayout> descriptor_layouts_;
};

