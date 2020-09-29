#pragma once

#include <unordered_map>
#include <stdexcept>
#include <memory>

#include <glm/glm.hpp>

#include "Mesh.hpp"
#include "Uniforms.hpp"
#include "VulkanTexture.hpp"
#include "Vertex.hpp"
#include "Animation.hpp"
#include "Skeleton.hpp"
#include "Globals.hpp"
#include "PipelineSettings.hpp"

class Model {
    
public:
    Model(std::vector<std::shared_ptr<Mesh>> meshes, std::unordered_map<std::string, Animation> animations,
          std::shared_ptr<PipelineSettings> pipelineSettings, std::shared_ptr<Uniforms<LocalTransform>> uniforms,
          std::unique_ptr<Skeleton> skeleton, VulkanDevice& device)
            : position(), rotation(), scale(1.0f, 1.0f, 1.0f),
              meshes(meshes), animations(animations), uniforms(uniforms),
              pipelineSettings(pipelineSettings), skeleton(std::move(skeleton)) {
    }
    
    inline std::vector<std::shared_ptr<Mesh>>& getMeshes() { return meshes; }
    inline Uniforms<LocalTransform>& getUniforms() { return *uniforms; }
    inline Pipeline& getPipeline() { return *pipeline; }
    inline Pipeline& getShadowPipeline() { return *shadowPipeline; }
    inline bool hasShadows() { return pipelineSettings->shadowVertexShader != ""; }
    inline PipelineSettings& getPipelineSettings() { return *pipelineSettings; }
    inline void setPipeline(std::shared_ptr<Pipeline> pipeline) { this->pipeline = pipeline; }
    inline void setShadowPipeline(std::shared_ptr<Pipeline> shadowPipeline) { this->shadowPipeline = shadowPipeline; }
    inline void cleanupPipelines() {
        pipeline.reset();
        shadowPipeline.reset();
    }
    
    void playAnimation(std::string name, float time, int meshIndex = 0) {
        if (animations.count(name) == 0) {
            throw std::runtime_error("The animation '" + name + "' doesn't exist.");
        }
        animations[name].evaluate(*meshes[meshIndex], *skeleton, time);
    }

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    
private:
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::unordered_map<std::string, Animation> animations;
    std::shared_ptr<Uniforms<LocalTransform>> uniforms;
    std::shared_ptr<Pipeline> pipeline;
    std::shared_ptr<Pipeline> shadowPipeline;
    std::shared_ptr<PipelineSettings> pipelineSettings;
    std::unique_ptr<Skeleton> skeleton;

};
