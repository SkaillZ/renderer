#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"

class VulkanSwapchain {
    
public:
    VulkanSwapchain(VulkanDevice& vulkanDevice);
    virtual ~VulkanSwapchain();
    
    size_t imageNumber() { return images.size(); }
    
    VkSwapchainKHR swapChain;
    std::vector<VkImage> images;
    VkFormat imageFormat;
    VkFormat depthFormat;
    VkExtent2D extent;
    std::vector<VkImageView> imageViews;
    
private:
    VulkanDevice& vulkanDevice;
    
    void createSwapChain();
    void createImageViews();
    
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VulkanDevice::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    
};
