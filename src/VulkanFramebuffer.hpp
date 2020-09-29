#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"
#include "VulkanRenderPasses.hpp"
#include "VulkanSwapchain.hpp"

class VulkanFramebuffer {
    
public:
    VulkanFramebuffer(VulkanDevice& vulkanDevice, VulkanRenderPasses& renderPass, VulkanSwapchain &swapchain, VkExtent2D extent);
    virtual ~VulkanFramebuffer();
   
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkFramebuffer shadowFramebuffer;
    
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
    VkImage shadowDepthImage;
    VkDeviceMemory shadowDepthImageMemory;
    VkImageView shadowDepthImageView;
    VkSampler shadowSampler;
    
    inline static const uint32_t SHADOWMAP_SIZE = 2048;
    
private:
    
    void createColorResources();
    void createDepthResources();
    void createShadowDepthResources();
    
    void createSwapchainFramebuffers();
    void createShadowFramebuffer();
    
    VulkanDevice& vulkanDevice;
    VulkanRenderPasses& renderPass;
    VulkanSwapchain& swapchain;
    VkExtent2D extent;
};
