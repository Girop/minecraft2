#include <array>
#include "descriptors.hpp"
#include "device.hpp"
#include "utils.hpp"

namespace 
{

VkDescriptorPool allocate_descriptor_pool(Device& device, uint32_t const frame_count)
{
    std::array sizes
    {
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame_count},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frame_count},
    };

    VkDescriptorPoolCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.maxSets = static_cast<uint32_t>(frame_count * sizes.size() + 4);
    info.poolSizeCount = sizes.size();
    info.pPoolSizes = sizes.data();

    VkDescriptorPool pool;
    utils::check_vk(vkCreateDescriptorPool(device.logical(), &info, nullptr, &pool));
    return pool;
}

} // namespace


DescriptorPool::DescriptorPool(Device& device, uint32_t const capacity) :
    device_{device},
    pool_{allocate_descriptor_pool(device, capacity)}
{}


std::vector<VkDescriptorSet> DescriptorPool::allocate_descriptor_sets(VkDescriptorSetLayout const layout, uint32_t const number) const
{   
    std::vector<VkDescriptorSetLayout> layouts;
    layouts.resize(number);
    for (auto& lyt : layouts) 
    {
        lyt = layout;
    }
    VkDescriptorSetAllocateInfo alloc_info
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool_,
        .descriptorSetCount = number,
        .pSetLayouts = layouts.data(),
    };

    std::vector<VkDescriptorSet> sets;
    sets.resize(number);
    utils::check_vk(vkAllocateDescriptorSets(device_.logical(), &alloc_info, sets.data()));
    return sets;
}

void update_uniform_descriptor(Device const& device, VkDescriptorSet const set, GpuBuffer const& buffer) 
{
    VkDescriptorBufferInfo buffer_info{
        .buffer = buffer.handle(),
        .offset = 0,
        .range = buffer.size()
    };
    VkWriteDescriptorSet desc_write
    {
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
    vkUpdateDescriptorSets(device.logical(), 1, &desc_write, 0, nullptr);
}

void update_texture_descriptor(Device const& device, VkDescriptorSet const set, Texture const& texture, VkSampler const sampler) 
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = texture.image().view();
    imageInfo.sampler = sampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = set;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device.logical(), 1, &descriptorWrite, 0, nullptr);
}
