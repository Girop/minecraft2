#include "framedata.hpp"
#include "uniforms.hpp"
#include "queues.hpp"
#include "device.hpp"

Framedata::Framedata(Device& device, VkSurfaceKHR const surface) :
    uniform{
        device,
        sizeof(UniformBufferObject),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
        cmd{device, surface, QueueFamily{device.physical(), surface}.get_queue(device.logical())},
        image_acquired{device},
        image_rendered{device}
{}


Frames::Frames( 
    Device& device,
    VkSurfaceKHR const surface,
    std::vector<VkDescriptorSet> const& texture_sets,
    std::vector<VkDescriptorSet> const& uniform_sets,
    Texture const& texture,
    VkSampler const sampler) :
    data{Framedata{device, surface}, Framedata{device, surface}} 
{
    assert(texture_sets.size() == FRAME_OVERLAP);
    assert(uniform_sets.size() == FRAME_OVERLAP);

    for (size_t idx {}; idx < FRAME_OVERLAP; ++idx)
    {
        data[idx].texture_descriptor = texture_sets.at(idx);
        data[idx].unfirom_descriptor = uniform_sets.at(idx);
    }

    for (auto& frame : data) 
    {
        update_uniform_descriptor(device, frame.unfirom_descriptor, frame.uniform);
        update_texture_descriptor(device, frame.texture_descriptor, texture, sampler);
    }
}
