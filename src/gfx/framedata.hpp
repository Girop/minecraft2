#pragma once
#include "buffer.hpp"
#include "commands.hpp"
#include "descriptors.hpp"
#include "semaphore.hpp"

constexpr uint8_t FRAME_OVERLAP {2u};

class Device;

struct Framedata 
{
    Framedata(Device& device, VkSurfaceKHR const surface);
    GpuBuffer uniform;
    CommandBuffer cmd;
    Semaphore image_acquired;
    Semaphore image_rendered;
    VkDescriptorSet texture_descriptor; 
    VkDescriptorSet unfirom_descriptor;
};

// TODO move descriptor logic setup to some other place
struct Frames
{
    Frames(
        Device& device,
        VkSurfaceKHR const surface,
        std::vector<VkDescriptorSet> const& texture_sets,
        std::vector<VkDescriptorSet> const& uniform_sets,
        Texture const& texture,
        VkSampler const sampler);
    std::array<Framedata, FRAME_OVERLAP> data;
};

