#pragma once

#include <array>

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"

class VulkanRenderPasses {
    
public:
    VulkanRenderPasses(VulkanDevice& vulkanDevice, VkFormat imageFormat, VkFormat depthFormat);
    virtual ~VulkanRenderPasses();
    
    inline static const VkFormat SHADOWS_DEPTH_FORMAT = VK_FORMAT_D16_UNORM;
    
    VulkanDevice& vulkanDevice;
    VkRenderPass shadowsRenderPass;
    VkRenderPass mainRenderPass;
    
private:
    VkFormat imageFormat;
    VkFormat depthFormat;
    
    void createShadowsRenderPass();
    void createMainRenderPass();
};
