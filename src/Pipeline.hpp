#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderPasses.hpp"
#include "PipelineSettings.hpp"

class Pipeline {
    
public:
    Pipeline(VulkanRenderPasses& renderPass, VkDescriptorSetLayout descriptorSetLayout, VkExtent2D extent, PipelineSettings& settings, bool shadowPipeline);
    virtual ~Pipeline();
    
    void bind(VkCommandBuffer commandBuffer);
    VkPipelineLayout getLayout() { return layout; }
    
private:
    VkShaderModule createShaderModule(const std::vector<char>& code);
    
    VulkanRenderPasses& renderPass;
    
    VkPipelineLayout layout;
    VkPipeline graphicsPipeline;
    
};
