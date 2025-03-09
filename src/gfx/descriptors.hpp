#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "buffer.hpp"
#include "texture.hpp"

class Device;

class DescriptorPool {
public:
    DescriptorPool(Device& device, uint32_t const frame_overlap);

    std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout const layout, uint32_t const number) const;
private:
    Device const& device_;
    VkDescriptorPool pool_;  
};

void update_uniform_descriptor(Device const& device, VkDescriptorSet const set, GpuBuffer const& buffer);
void update_texture_descriptor(Device const& device, VkDescriptorSet const set, Texture const& texture, VkSampler const sampler);
