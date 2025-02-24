#pragma once
#include <vector>
#include "viewport.hpp"
#include "vertex.hpp"

struct Pipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};


// TODO figure out builder on temporaries
class PipelineBuilder {
public:
    PipelineBuilder();
    Pipeline build(VkDevice device) const;

    PipelineBuilder& set_descriptor_sets(VkDescriptorSetLayout ubo, VkDescriptorSetLayout texture);
    PipelineBuilder& set_vertex(VkShaderModule vertex);
    PipelineBuilder& set_fragment(VkShaderModule fragment);
    PipelineBuilder& set_viewport(Viewport const& viewport);
    PipelineBuilder& set_render_pass(VkRenderPass render_pass) {
        render_pass_ = render_pass; 
        return *this;
    }
    PipelineBuilder& set_descriptors(
        VkVertexInputBindingDescription binding,
        Vertex::AttributeDescriptions attribute
    );
    PipelineBuilder& set_depth_testing(VkPipelineDepthStencilStateCreateInfo depthstencil) {
        depth_create_info_ = depthstencil;
        return *this;
    }

private:
    VkPipelineInputAssemblyStateCreateInfo assembly_info() const;
    VkPipelineRasterizationStateCreateInfo rasterizer_info() const;
    VkPipelineMultisampleStateCreateInfo multisampling_info() const;
    VkPipelineDynamicStateCreateInfo dynamic_state_info() const;
    VkPipelineColorBlendStateCreateInfo blend_info() const;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_;
    VkShaderModule vertex_;
    VkShaderModule fragment_;

    struct {
        Viewport viewport;
        VkPipelineViewportStateCreateInfo info;
    } viewport_;

    VkPipelineColorBlendAttachmentState attachment_;
    VkRenderPass render_pass_;
    
    VkPipelineDepthStencilStateCreateInfo depth_create_info_;

    struct {
        VkPipelineVertexInputStateCreateInfo info;
        VkVertexInputBindingDescription binding_desriptions;
        Vertex::AttributeDescriptions attribute_descriptions;
    } inpute_vertex_;
    
    std::vector<VkDescriptorSetLayout> descriptor_layouts_;
};

