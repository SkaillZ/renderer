#include "Mesh.hpp"

#include "VulkanUtils.hpp"

Mesh::Mesh(VulkanDevice& device, std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::unordered_map<std::string, MeshBoneData> boneData)
        : device(device), vertices(vertices), indices(indices), boneData(boneData) {
            
    boneTransforms.resize(boneData.size(), glm::mat4(1.0f));
    createVertexBuffer();
    createIndexBuffer();
}

Mesh::~Mesh() {
    device.freeBuffer(indexBuffer);
    device.freeBuffer(vertexBuffer);
}

void Mesh::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    VulkanBuffer stagingBuffer;
    device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
    
    void* data;
    vkMapMemory(device.device, stagingBuffer.memory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device.device, stagingBuffer.memory);
    
    device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);
    
    VulkanUtils::copyBuffer(device, stagingBuffer.buffer, vertexBuffer.buffer, bufferSize);
    
    device.freeBuffer(stagingBuffer);
}

void Mesh::updateVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    VulkanBuffer stagingBuffer;
    device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
    
    void* data;
    vkMapMemory(device.device, stagingBuffer.memory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device.device, stagingBuffer.memory);
    
    VulkanUtils::copyBuffer(device, stagingBuffer.buffer, vertexBuffer.buffer, bufferSize);
    
    device.freeBuffer(stagingBuffer);
}

void Mesh::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    VulkanBuffer stagingBuffer;
    device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
    
    void* data;
    vkMapMemory(device.device, stagingBuffer.memory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device.device, stagingBuffer.memory);
    
    device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);
    
    VulkanUtils::copyBuffer(device, stagingBuffer.buffer, indexBuffer.buffer, bufferSize);
    
    device.freeBuffer(stagingBuffer);
}

void Mesh::bindBuffers(VkCommandBuffer commandBuffer) {
    VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::draw(VkCommandBuffer commandBuffer) {
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Mesh::calculateTangents() {
    std::vector<glm::vec3> bitangents(vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i+=3) {
        Vertex& v0 = vertices[indices[i+0]];
        Vertex& v1 = vertices[indices[i+1]];
        Vertex& v2 = vertices[indices[i+2]];
        
        glm::vec2 tex0 = v0.texCoord;
        glm::vec2 tex1 = v1.texCoord;
        glm::vec2 tex2 = v2.texCoord;
        
        glm::vec3 edge1 = v1.pos - v0.pos;
        glm::vec3 edge2 = v2.pos - v0.pos;
        
        glm::vec2 uv1 = tex1 - tex0;
        glm::vec2 uv2 = tex2 - tex0;
        
        float f = 1.0f / (uv1.x * uv2.y - uv1.y * uv2.x);
        
        glm::vec4 tangent(
            ((edge1.x * uv2.y) - (edge2.x * uv1.y)) * f,
            ((edge1.y * uv2.y) - (edge2.y * uv1.y)) * f,
            ((edge1.z * uv2.y) - (edge2.z * uv1.y)) * f,
            0.0f
        );
        
        glm::vec3 bitangent(
            ((edge1.x * uv2.x) - (edge2.x * uv1.x)) * f,
            ((edge1.y * uv2.x) - (edge2.y * uv1.x)) * f,
            ((edge1.z * uv2.x) - (edge2.z * uv1.x)) * f
        );
        
        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;
        
        bitangents[i+0] += bitangent;
        bitangents[i+1] += bitangent;
        bitangents[i+2] += bitangent;
    }
    
    for (size_t i = 0; i < vertices.size(); i++) {
        glm::vec3 n = vertices[i].normal;
        glm::vec3 tangent = vertices[i].tangent;
        glm::vec3 bitangent = bitangents[i];
        
        glm::vec3 t = tangent - (n * glm::dot(n, tangent));
        t = normalize(t);
        
        glm::vec3 c = glm::cross(n, tangent);
        float w = (glm::dot(c, bitangent) < 0) ? 1.0f : -1.0f;
        vertices[i].tangent = glm::vec4(t.x, t.y, t.z, w);
    }
}

std::vector<std::array<glm::vec3, 3>> Mesh::getAllTriangles() {
    std::vector<std::array<glm::vec3, 3>> triangles;
    triangles.resize(indices.size() / 3);
    for (size_t i = 0; i < indices.size() / 3; i++) {
        triangles[i][0] = vertices[indices[i*3]].pos;
        triangles[i][1] = vertices[indices[i*3+1]].pos;
        triangles[i][2] = vertices[indices[i*3+2]].pos;
    }

    return triangles;
}
