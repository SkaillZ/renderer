#pragma once

#include <string>
#include <array>

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"

class VulkanTexture {
    
public:
    VulkanTexture(std::string path, VulkanDevice& device, bool srgb);
    VulkanTexture(VulkanDevice& device, bool srgb);
    ~VulkanTexture();
    
    VkDescriptorImageInfo getDescriptorImageInfo();
    
    static std::shared_ptr<VulkanTexture> loadCubemap(std::array<std::string, 6> paths, VulkanDevice& device, bool srgb);
    
private:
    bool srgb;
    bool cubeMap;
    uint32_t mipLevels = 1;
    VkImage textureImage = nullptr;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    
    VulkanDevice& device;

    void createTextureImage(std::string path);
    void createCubemapTextureImage(std::array<std::string, 6> paths);
    void createTextureImageView();
    void createTextureSampler();
    
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
};
