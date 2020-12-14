#pragma once

#include <optional>
#include <vector>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanBuffer.hpp"

class VulkanDevice {
    
public:
    
    VulkanDevice(GLFWwindow* window);
    virtual ~VulkanDevice();
    
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkCommandPool commandPool;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    VkSampleCountFlagBits userRequestedMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlagBits maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    
#ifdef NDEBUG
    bool enableValidationLayers = false;
#else
    bool enableValidationLayers = true;
#endif
    
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VulkanBuffer& buffer);
    void freeBuffer(VulkanBuffer& buffer);
    
    static const std::vector<const char*> validationLayers;
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    VkSampleCountFlagBits getMsaaSamples();
    int getUserRequestedMsaaSamples() { return (int) userRequestedMsaaSamples; }
    void setUserRequestedMsaaSamples(int sampleCount) { userRequestedMsaaSamples = (VkSampleCountFlagBits) sampleCount; }
    
private:
    static const std::vector<const char*> deviceExtensions;
    
    void createInstance();
    void createSurface();
    void setupDebugMessenger();
    
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();
    
    bool checkValidationLayerSupport();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSampleCountFlagBits getMaxMsaaSamples();
};
