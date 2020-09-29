#include "VulkanFramebuffer.hpp"

#include "VulkanUtils.hpp"

VulkanFramebuffer::VulkanFramebuffer(VulkanDevice& vulkanDevice, VulkanRenderPasses& renderPass, VulkanSwapchain& swapchain, VkExtent2D extent)
        : vulkanDevice(vulkanDevice), renderPass(renderPass), swapchain(swapchain), extent(extent) {
    createColorResources();
    createDepthResources();
    createShadowDepthResources();
            
    createSwapchainFramebuffers();
    createShadowFramebuffer();
}

VulkanFramebuffer::~VulkanFramebuffer() {
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(vulkanDevice.device, framebuffer, nullptr);
    }
    
    vkDestroyFramebuffer(vulkanDevice.device, shadowFramebuffer, nullptr);
    
    vkDestroyImageView(vulkanDevice.device, shadowDepthImageView, nullptr);
    vkDestroyImage(vulkanDevice.device, shadowDepthImage, nullptr);
    vkFreeMemory(vulkanDevice.device, shadowDepthImageMemory, nullptr);

    vkDestroyImageView(vulkanDevice.device, colorImageView, nullptr);
    vkDestroyImage(vulkanDevice.device, colorImage, nullptr);
    vkFreeMemory(vulkanDevice.device, colorImageMemory, nullptr);

    vkDestroySampler(vulkanDevice.device, shadowSampler, nullptr);
    vkDestroyImageView(vulkanDevice.device, depthImageView, nullptr);
    vkDestroyImage(vulkanDevice.device, depthImage, nullptr);
    vkFreeMemory(vulkanDevice.device, depthImageMemory, nullptr);
}

void VulkanFramebuffer::createColorResources() {
    VkFormat colorFormat = swapchain.imageFormat;
    
    VulkanUtils::createImage(vulkanDevice.device, vulkanDevice.physicalDevice, extent.width, extent.height, 1, false, vulkanDevice.maxMsaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = VulkanUtils::createImageView(vulkanDevice.device, colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    
    VulkanUtils::transitionImageLayout(vulkanDevice, colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}

void VulkanFramebuffer::createDepthResources() {
    VkFormat depthFormat = swapchain.depthFormat;
    VulkanUtils::createImage(vulkanDevice.device, vulkanDevice.physicalDevice, extent.width, extent.height, 1, false, vulkanDevice.maxMsaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = VulkanUtils::createImageView(vulkanDevice.device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    
    VulkanUtils::transitionImageLayout(vulkanDevice, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void VulkanFramebuffer::createShadowDepthResources() {
    VkFormat depthFormat = VulkanRenderPasses::SHADOWS_DEPTH_FORMAT;
    VulkanUtils::createImage(vulkanDevice.device, vulkanDevice.physicalDevice, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, false, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT  | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowDepthImage, shadowDepthImageMemory);
    shadowDepthImageView = VulkanUtils::createImageView(vulkanDevice.device, shadowDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    
   // VulkanUtils::transitionImageLayout(vulkanDevice, shadowDepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1);
}

void VulkanFramebuffer::createSwapchainFramebuffers() {
    swapChainFramebuffers.resize(swapchain.imageViews.size());
    
    for (size_t i = 0; i < swapchain.imageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapchain.imageViews[i]
        };
        
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass.mainRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(vulkanDevice.device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void VulkanFramebuffer::createShadowFramebuffer() {
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass.shadowsRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &shadowDepthImageView;
    framebufferInfo.width = SHADOWMAP_SIZE;
    framebufferInfo.height = SHADOWMAP_SIZE;
    framebufferInfo.layers = 1;
    
    if (vkCreateFramebuffer(vulkanDevice.device, &framebufferInfo, nullptr, &shadowFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
    
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    
    if (vkCreateSampler(vulkanDevice.device, &samplerInfo, nullptr, &shadowSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow sampler");
    }
}
