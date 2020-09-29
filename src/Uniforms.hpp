#pragma once

#include <map>

#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanTexture.hpp"
#include "VulkanFramebuffer.hpp"
#include "Pipeline.hpp"
#include "Globals.hpp"

template <typename TUniformStruct>
class Uniforms {
    
public:
    TUniformStruct ubo;
    
    Uniforms(VulkanDevice& device, size_t textureNum, bool addShadowMaps) : ubo(), addShadowMaps(addShadowMaps), device(device) {
        createDescriptorSetLayout(textureNum);
    }
    
    virtual ~Uniforms() {
        vkDestroyDescriptorSetLayout(device.device, descriptorSetLayout, nullptr);
    }
    
    void initializeDescriptors(VulkanSwapchain& swapchain, VulkanFramebuffer& framebuffer) {
        swapChainImageNumber = swapchain.imageNumber();
        
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets(framebuffer);
    }
    
    void destroyDescriptors() {
        // Swapchain already destroyed, how should we get the number of images?
        for (size_t i = 0; i < swapChainImageNumber; i++) {
            device.freeBuffer(uniformBuffers[i]);
            device.freeBuffer(globalsBuffers[i]);
        }
        
        vkDestroyDescriptorPool(device.device, descriptorPool, nullptr);
    }
    
    void bind(VkCommandBuffer commandBuffer, Pipeline& pipeline, size_t descriptorSetIndex) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 1, &descriptorSets[descriptorSetIndex], 0, nullptr);
    }
    
    void update(size_t currentImage, Globals& globals) {
        void* data;
        vkMapMemory(device.device, uniformBuffers[currentImage].memory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device.device, uniformBuffers[currentImage].memory);
        
        vkMapMemory(device.device, globalsBuffers[currentImage].memory, 0, sizeof(globals), 0, &data);
        memcpy(data, &globals, sizeof(globals));
        vkUnmapMemory(device.device, globalsBuffers[currentImage].memory);
    }
    
    void addTexture(uint32_t binding, std::shared_ptr<VulkanTexture> texture) { this->textures[binding] = texture; }
    
    VkDescriptorSetLayout getDescriptorSetLayout() { return descriptorSetLayout; }
    
private:
    std::map<uint32_t, std::shared_ptr<VulkanTexture>> textures;
    bool addShadowMaps;
    
    VkDescriptorSetLayout descriptorSetLayout;
    VulkanDevice& device;
    size_t swapChainImageNumber;
    
    std::vector<VulkanBuffer> uniformBuffers;
    
    // TODO: only use a single memory per swap chain image instead of per object
    std::vector<VulkanBuffer> globalsBuffers;
    
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    
    void createDescriptorSetLayout(size_t textureNum) {
        if (addShadowMaps) {
            textureNum++;
        }
        
        std::vector<VkDescriptorSetLayoutBinding> bindings(textureNum + 2);
        
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].pImmutableSamplers = nullptr;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        
        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].pImmutableSamplers = nullptr;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        for (int i = 2; i < bindings.size(); i++) {
            bindings[i].binding = i;
            bindings[i].descriptorCount = 1;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].pImmutableSamplers = nullptr;
            bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        
        if (vkCreateDescriptorSetLayout(device.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }
    
    void createUniformBuffers() {
        uniformBuffers.resize(swapChainImageNumber);
        
        globalsBuffers.resize(swapChainImageNumber);
        
        for (size_t i = 0; i < swapChainImageNumber; i++) {
            device.createBuffer(sizeof(TUniformStruct), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i]);
            device.createBuffer(sizeof(Globals), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, globalsBuffers[i]);
        }
    }
    
    void createDescriptorPool() {
        size_t textureNum = textures.size();
        if (addShadowMaps) {
            textureNum++;
        }
        
        std::vector<VkDescriptorPoolSize> poolSizes(textureNum > 0 ? 2 : 1);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(2 * swapChainImageNumber);
        
        if (textureNum > 0) {
            poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[1].descriptorCount = static_cast<uint32_t>(textureNum * swapChainImageNumber);
        }
        
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapChainImageNumber);
        
        if (vkCreateDescriptorPool(device.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }
    
    void createDescriptorSets(VulkanFramebuffer& framebuffer) {
        size_t textureNum = textures.size();
        if (addShadowMaps) {
            textureNum++;
        }
        
        std::vector<VkDescriptorSetLayout> layouts(swapChainImageNumber, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImageNumber);
        allocInfo.pSetLayouts = layouts.data();
        
        descriptorSets.resize(swapChainImageNumber);
        if (vkAllocateDescriptorSets(device.device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
        
        for (size_t i = 0; i < swapChainImageNumber; i++) {
            VkDescriptorBufferInfo localTransformInfo = {};
            localTransformInfo.buffer = uniformBuffers[i].buffer;
            localTransformInfo.offset = 0;
            localTransformInfo.range = sizeof(TUniformStruct);
            
            VkDescriptorBufferInfo globalsInfo = {};
            globalsInfo.buffer = globalsBuffers[i].buffer;
            globalsInfo.offset = 0;
            globalsInfo.range = sizeof(Globals);
            
            std::vector<VkWriteDescriptorSet> descriptorWrites(textureNum + 2);
            
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &localTransformInfo;
            
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &globalsInfo;
            
            std::vector<VkDescriptorImageInfo> imageInfos(textures.size());
            
            for (int j = 2; j < textures.size() + 2; j++) {
                imageInfos[j-2] = textures[j]->getDescriptorImageInfo();
            }
            
            for (int j = 2; j < textures.size() + 2; j++) {
                auto& imageInfo = imageInfos[j-2];
                
                descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[j].dstSet = descriptorSets[i];
                descriptorWrites[j].dstBinding = j;
                descriptorWrites[j].dstArrayElement = 0;
                descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[j].descriptorCount = 1;
                descriptorWrites[j].pImageInfo = &imageInfo;
            }
            
            if (addShadowMaps) {
                // Add the shadow map at the end
                VkDescriptorImageInfo imageInfo = {};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                imageInfo.imageView = framebuffer.shadowDepthImageView;
                imageInfo.sampler = framebuffer.shadowSampler;
                
                size_t index = textures.size() + 2;
                descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[index].dstSet = descriptorSets[i];
                descriptorWrites[index].dstBinding = static_cast<uint32_t>(index);
                descriptorWrites[index].dstArrayElement = 0;
                descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[index].descriptorCount = 1;
                descriptorWrites[index].pImageInfo = &imageInfo;
            }
            
            vkUpdateDescriptorSets(device.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
    
};
