#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "Vertex.hpp"

struct MeshBoneData {
    std::string name;
    uint32_t index;
    glm::mat4 offset;
    
    MeshBoneData() = default; // Required for unordered_map
    MeshBoneData(std::string name, uint32_t index, glm::mat4 offset) : name(name), index(index), offset(offset) {}
};

class Mesh {
    
public:
    Mesh(VulkanDevice& device, std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::unordered_map<std::string, MeshBoneData> boneData);
    virtual ~Mesh();
    
    void bindBuffers(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);
    void calculateTangents();
    
    MeshBoneData& getBoneData(std::string boneName) { return boneData[boneName]; }
    std::unordered_map<std::string, MeshBoneData>& getBoneData() { return boneData; }
    std::vector<glm::mat4>& getBoneTransforms() { return boneTransforms; }
    
private:
    VulkanDevice& device;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<std::string, MeshBoneData> boneData;
    
    // TODO: allow for seperate transforms with reused meshes
    std::vector<glm::mat4> boneTransforms;
    
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    
    void createVertexBuffer();
    void createIndexBuffer();
};
