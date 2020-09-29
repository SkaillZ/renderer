#pragma once

#include <vulkan/vulkan.h>
#include "VulkanDevice.hpp"

// TODO: use an allocator
struct VulkanBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
};
