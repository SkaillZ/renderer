#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <chrono>
#include <vector>

#include "Vertex.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanRenderPasses.hpp"
#include "VulkanFramebuffer.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Globals.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {
    
public:
    Renderer(GLFWwindow *window);
    virtual ~Renderer();
    
    void finishInitialization();
    void setFramebufferResized() { framebufferResized = true; }
    void waitForDeviceIdle() { vkDeviceWaitIdle(vulkanDevice->device); }
    
    Camera& getCamera() { return camera; }
    Globals& getGlobals() { return globals; }
    VulkanDevice& getDevice() { return *vulkanDevice; }

    void addModel(std::shared_ptr<Model>& model) { models.push_back(model); }
    
    void setLight(size_t index, Light light);
    void setAmbientLight(glm::vec3 color) { globals.ambientColor = color; };

    glm::vec2 getExtent() { return { swapChain->extent.width, swapChain->extent.height }; };
    
    void recreateSwapChain(bool waitForEvent = false);
    void drawFrame();
    
private:
    GLFWwindow* window;
    
    std::unique_ptr<VulkanDevice> vulkanDevice = nullptr;
    std::unique_ptr<VulkanSwapchain> swapChain = nullptr;
    std::unique_ptr<VulkanRenderPasses> renderPass = nullptr;
    std::unique_ptr<VulkanFramebuffer> framebuffer = nullptr;
    std::vector<std::shared_ptr<Model>> models;
    
    std::vector<VkCommandBuffer> commandBuffers;
    
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    Camera camera;
    Globals globals;
    size_t currentFrame = 0;
    
    bool framebufferResized = false;
    
    void initVulkan();
    void cleanupSwapChain();
    void cleanup();
    void createFramebuffers();
    void createModelPipelines();
    void createCommandBuffers();
    void createSyncObjects();
    void updateUniforms(uint32_t currentImage);
    
};
