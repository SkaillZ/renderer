#pragma once

#include <array>

#include <vulkan/vulkan.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex {
    static const size_t BONES_PER_VERTEX = 4;
    
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 color;
    glm::vec2 texCoord;
    glm::vec4 tangent;
    uint32_t boneIds[BONES_PER_VERTEX];
    float boneWeights[BONES_PER_VERTEX];
    
    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 7> getAttributeDescriptions();
    
    bool operator==(const Vertex& other) const;
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                    (hash<glm::vec4>()(vertex.color) << 1)) >> 1) ^
                    (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
