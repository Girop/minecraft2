#include <assert.h>
#include <span>
#include <set>
#include "utility.hpp"
#include "queues.hpp"
#include "swapchain.hpp"
#include "device.hpp"


constexpr std::array DEVICE_EXTENSIONS {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};


VkDeviceCreateInfo device_create_info(std::span<VkDeviceQueueCreateInfo const> create_infos)
{
    VkPhysicalDeviceFeatures dev_features{};
    dev_features.fillModeNonSolid = VK_TRUE;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(create_infos.size());
    device_create_info.pQueueCreateInfos = create_infos.data();
    device_create_info.enabledExtensionCount = DEVICE_EXTENSIONS.size();
    device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
    device_create_info.pEnabledFeatures = &dev_features;

    return device_create_info;
}


class DeviceFinder
{
  public:
    DeviceFinder(VkInstance instance, VkSurfaceKHR surface) :
        instance_{instance},
        surface_{surface}
    {}

    VkPhysicalDevice find() const
    {
        uint32_t device_count{};
        vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
        std::vector<VkPhysicalDevice> devices;
        devices.resize(device_count);
        vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
        VkPhysicalDevice device{VK_NULL_HANDLE};
        for (auto phys_device : devices)
        {
            if (!suitable(phys_device))
                continue;
            device = phys_device;
            break;
        }
        assert(device != VK_NULL_HANDLE);
        return device;
    }

    VkDevice logical(VkPhysicalDevice phys_device) const
    {
        auto indicies = QueueFamilyIndicies::from(phys_device, surface_);
        std::array families{indicies.graphics.value(), indicies.present.value()};
        std::vector<VkDeviceQueueCreateInfo> create_infos;
        create_infos.resize(families.size());

        float queue_prio{1.0f};
        for (size_t idx{}; idx < create_infos.size(); ++idx)
        {
            create_infos[idx] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, families[idx], 1, &queue_prio};
        }

        auto device_info = device_create_info(create_infos);
        VkDevice device;
        check_vk(vkCreateDevice(phys_device, &device_info, nullptr, &device));
        return device;
    }

  private:
    bool suitable(VkPhysicalDevice phys_device) const
    {
        return supports_extensions(phys_device) and is_external_gpu(phys_device) and
               QueueFamilyIndicies::from(phys_device, surface_).complete() and
               SwapChainSupportDetails::create(phys_device, surface_).supported();
    }

    bool is_external_gpu(VkPhysicalDevice device) const
    {
        VkPhysicalDeviceProperties device_props;
        VkPhysicalDeviceFeatures dev_features;

        vkGetPhysicalDeviceProperties(device, &device_props);
        vkGetPhysicalDeviceFeatures(device, &dev_features);

        return (device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU or
                device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) and
               dev_features.geometryShader;
    }

    bool supports_extensions(VkPhysicalDevice device) const
    {
        uint32_t extension_count{};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

        std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
        for (auto const& extension : extensions)
        {
            required_extensions.erase(extension.extensionName);
        }
        return required_extensions.empty();
    }

    VkInstance instance_;
    VkSurfaceKHR surface_;
};


Device Device::create(VkInstance instance, VkSurfaceKHR surface) {
    DeviceFinder device_finder {instance, surface};
    const auto phys_device {device_finder.find()};
    const auto logical {device_finder.logical(phys_device)};
    return {
        phys_device,
        logical
    };
}

