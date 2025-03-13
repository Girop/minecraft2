#include <algorithm>
#include <assert.h>
#include <span>
#include "queues.hpp"
#include "swapchain.hpp"
#include "device.hpp"

namespace 
{

constexpr std::array DEVICE_REQUIRED_EXTENSIONS 
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};


VkDeviceCreateInfo device_create_info(std::span<VkDeviceQueueCreateInfo const> create_infos)
{
    // VkPhysicalDeviceFeatures dev_features{};
    // dev_features.fillModeNonSolid = VK_TRUE;
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(create_infos.size());
    device_create_info.pQueueCreateInfos = create_infos.data();
    device_create_info.enabledExtensionCount = DEVICE_REQUIRED_EXTENSIONS.size();
    device_create_info.ppEnabledExtensionNames = DEVICE_REQUIRED_EXTENSIONS.data();
    device_create_info.pEnabledFeatures = nullptr;
    return device_create_info;
}

bool supports_extensions(VkPhysicalDevice const device)
{
    uint32_t extension_count{};
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> supported_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, supported_extensions.data());

    for (auto& extension : DEVICE_REQUIRED_EXTENSIONS) 
    {
        auto find_it = std::find_if(supported_extensions.begin(), supported_extensions.end(), [&extension](auto& item) {
            return std::strcmp(extension, item.extensionName) == 0;
        });
        if (find_it == supported_extensions.end()) return false;
    }
    return true;
}

bool is_external_gpu(VkPhysicalDevice const device)
{
    VkPhysicalDeviceProperties device_props;
    VkPhysicalDeviceFeatures dev_features;

    vkGetPhysicalDeviceProperties(device, &device_props);
    vkGetPhysicalDeviceFeatures(device, &dev_features);
    return (device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) and dev_features.geometryShader;
}

bool suitable(VkPhysicalDevice const device, VkSurfaceKHR const surface)
{
    return 
        supports_extensions(device) 
        and is_external_gpu(device) 
        and QueueFamily {device, surface}.exists()
        and SwapChainSupportDetails::create(device, surface).supported();
}

VkPhysicalDevice best_physical_device(VkInstance const instance, VkSurfaceKHR const surface)
{
    uint32_t device_count{};
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    std::vector<VkPhysicalDevice> devices;
    devices.resize(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    auto const found_it = std::find_if(devices.begin(), devices.end(), [&surface](auto const& arg) {return suitable(arg, surface);});
    if (found_it == devices.end())
    {
        fail("No device connected");
    }
    return *found_it;
}

VkDevice best_logical_device(VkPhysicalDevice const phys_device, VkSurfaceKHR const surface) 
{

    QueueFamily family {phys_device, surface};
    assert(family.exists());

    std::array<VkDeviceQueueCreateInfo, 1> create_infos;
    std::array families {family.id()};

    float queue_prio{1.0f};
    for (size_t idx{}; idx < create_infos.size(); ++idx)
    {
        create_infos[idx] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, families[idx], 1, &queue_prio};
    }

    auto const device_info = device_create_info(create_infos);
    VkDevice device;
    utils::check_vk(vkCreateDevice(phys_device, &device_info, nullptr, &device));
    return device;
}

} // namespace


Device::Device(VkInstance const instance, VkSurfaceKHR const surface) :
    physical_{best_physical_device(instance, surface)},
    logical_{best_logical_device(physical_, surface)},
    queue_{QueueFamily {physical_, surface}.get_queue(logical_)},
    cmd_{*this, surface, queue_, false} // questionable, but correct
{}

uint32_t Device::find_memory_type_index(
    uint32_t const type_filter,
    VkMemoryPropertyFlags const props) const 
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_, &memory_properties);
    uint32_t res{0};
    for (uint32_t idx{0}; idx < memory_properties.memoryTypeCount; ++idx)
    {
        bool prop_pattern_matches = (memory_properties.memoryTypes[idx].propertyFlags & props) == props;
        bool type_matches = type_filter & (1 << idx);
        if (prop_pattern_matches and type_matches)
        {
            res = idx;
            break;
        }
    }

    if (res == 0) 
    {
        fail("Unable to find memory with given properties");
    }
    return res;
}

VkDeviceMemory Device::allocate(VkMemoryRequirements const mem_reqs, VkMemoryPropertyFlags const props) const 
{
    VkMemoryAllocateInfo alloc_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = find_memory_type_index(mem_reqs.memoryTypeBits, props)
    };
    VkDeviceMemory device_memory;
    utils::check_vk(vkAllocateMemory(logical_, &alloc_info, nullptr, &device_memory));
    return device_memory;
}

void Device::immediate_submit(std::function<void(VkCommandBuffer cmd)> const& func)
{
    cmd_.record(func, true);
    cmd_.submit_default();
    cmd_.wait();
}

void Device::wait() const
{
    vkDeviceWaitIdle(logical_);
}
