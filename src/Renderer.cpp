#include "Renderer.hpp"

Renderer::Renderer(GLFWwindow* window) {
    this->window = window;
    initVulkan();
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::initVulkan() {
    vulkanDevice = std::make_unique<VulkanDevice>(window);
    
    createFramebuffers();
}

void Renderer::finishInitialization() {
    createModelPipelines();
    
    createCommandBuffers();
    createSyncObjects();
}

void Renderer::cleanupSwapChain() {
    vkFreeCommandBuffers(vulkanDevice->device, vulkanDevice->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    
    for (auto& model : models) {
        model->cleanupPipelines();
    }
    framebuffer.reset();
    renderPass.reset();
    
    swapChain.reset();
    
    for (auto& model : models) {
        model->getUniforms().destroyDescriptors();
    }
}

void Renderer::cleanup() {
    cleanupSwapChain();
    
    for (auto& model : models) {
        model.reset();
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vulkanDevice->device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(vulkanDevice->device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(vulkanDevice->device, inFlightFences[i], nullptr);
    }
    
    vulkanDevice.reset();
}

void Renderer::recreateSwapChain(bool waitForEvent) {
    int width = 0, height = 0;
    while ((width == 0 || height == 0) && waitForEvent) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(vulkanDevice->device);
    
    cleanupSwapChain();
    
    createFramebuffers();
    
    createModelPipelines();
    createCommandBuffers();
}

void Renderer::createFramebuffers() {
    swapChain = std::make_unique<VulkanSwapchain>(*vulkanDevice);
    renderPass = std::make_unique<VulkanRenderPasses>(*vulkanDevice, swapChain->imageFormat, swapChain->depthFormat);
    framebuffer = std::make_unique<VulkanFramebuffer>(*vulkanDevice, *renderPass, *swapChain, swapChain->extent);
}

void Renderer::createModelPipelines() {
    for (auto& model : models) {
        auto mainPipeline = std::make_shared<Pipeline>(*renderPass, model->getUniforms().getDescriptorSetLayout(), swapChain->extent, model->getPipelineSettings(), false);
        
        model->setPipeline(mainPipeline);
        model->getUniforms().initializeDescriptors(*swapChain, *framebuffer);
        
        if (model->getPipelineSettings().shadowVertexShader != "") {
            // Shadows are disabled if no shadow vertex shader is set
            auto shadowPipeline = std::make_shared<Pipeline>(*renderPass, model->getUniforms().getDescriptorSetLayout(), VkExtent2D {VulkanFramebuffer::SHADOWMAP_SIZE, VulkanFramebuffer::SHADOWMAP_SIZE}, model->getPipelineSettings(), true);
            model->setShadowPipeline(shadowPipeline);
            model->getUniforms().initializeDescriptors(*swapChain, *framebuffer);
        }
    }
}

void Renderer::createCommandBuffers() {
    commandBuffers.resize(framebuffer->swapChainFramebuffers.size());
    
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkanDevice->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
    
    if (vkAllocateCommandBuffers(vulkanDevice->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    
    std::array<VkClearValue, 2> clearValues = {};
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
        
        // 1. Shadow Map Render Pass
        
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass->shadowsRenderPass;
        renderPassInfo.framebuffer = framebuffer->shadowFramebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {VulkanFramebuffer::SHADOWMAP_SIZE, VulkanFramebuffer::SHADOWMAP_SIZE};
        
        clearValues[0].depthStencil = {1.0f, 0};
        
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        for (auto& model : models) {
            if (!model->hasShadows()) {
                continue;
            }
            
            model->getShadowPipeline().bind(commandBuffers[i]);
            model->getUniforms().bind(commandBuffers[i], model->getShadowPipeline(), i);
            
            for (auto& mesh : model->getMeshes()) {
                mesh->bindBuffers(commandBuffers[i]);
                mesh->draw(commandBuffers[i]);
            }
        }
        
        vkCmdEndRenderPass(commandBuffers[i]);
        
        // 2. Main Render Pass
        
        renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass->mainRenderPass;
        renderPassInfo.framebuffer = framebuffer->swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain->extent;
        
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        for (auto& model : models) {
            model->getPipeline().bind(commandBuffers[i]);
            model->getUniforms().bind(commandBuffers[i], model->getPipeline(), i);
            
            for (auto& mesh : model->getMeshes()) {
                mesh->bindBuffers(commandBuffers[i]);
                mesh->draw(commandBuffers[i]);
            }
        }
        
        vkCmdEndRenderPass(commandBuffers[i]);
        
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void Renderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(vulkanDevice->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vulkanDevice->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vulkanDevice->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void Renderer::updateUniforms(uint32_t currentImage) {
    globals.viewPos = camera.position;

    auto viewMatrix = glm::toMat4(camera.rotation);
    viewMatrix = glm::translate(viewMatrix, camera.position);

    auto projectionMatrix = glm::perspective(glm::radians(camera.fovy), camera.aspectRatio, camera.nearPlane, camera.farPlane);
    projectionMatrix[1][1] *= -1;

    for (auto& model : models) {
        auto modelMatrix = glm::scale(glm::mat4(1.0f), model->scale);
        modelMatrix *= glm::toMat4(model->rotation);
        modelMatrix = glm::translate(modelMatrix, model->position);

        model->getUniforms().ubo.model = modelMatrix;
        model->getUniforms().ubo.view = viewMatrix;
        model->getUniforms().ubo.proj = projectionMatrix;
        model->getUniforms().update(currentImage, globals);
    }
}

void Renderer::drawFrame() {
    vkWaitForFences(vulkanDevice->device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vulkanDevice->device, swapChain->swapChain, std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain(true);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    
    updateUniforms(imageIndex);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkResetFences(vulkanDevice->device, 1, &inFlightFences[currentFrame]);
    
    if (vkQueueSubmit(vulkanDevice->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {swapChain->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(vulkanDevice->presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain(true);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::setLight(size_t index, Light light) {
    if (index >= MAX_LIGHTS)
        throw std::runtime_error("Invalid light index!");
    
    globals.lights[index] = light;
}
