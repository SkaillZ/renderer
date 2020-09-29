#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>

#include "VulkanDevice.hpp"

class VulkanUtils {
    
public:
    static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, bool cubeMap = false);
    static void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height,
                            uint32_t mipLevels, bool cubeMap, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
                            VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    static VkFormat findDisplayDepthFormat(VkPhysicalDevice physicalDevice);
    static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    static void transitionImageLayout(VulkanDevice& device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t baseArrayLayer = 0, uint32_t layerCount = 1);
    static bool hasStencilComponent(VkFormat format);
    static VkCommandBuffer beginSingleTimeCommands(VulkanDevice& device);
    static void endSingleTimeCommands(VkCommandBuffer commandBuffer, VulkanDevice& device);
    static void copyBuffer(VulkanDevice& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    static void copyBufferToImage(VulkanDevice& device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t arrayLayer = 0);
    
};
