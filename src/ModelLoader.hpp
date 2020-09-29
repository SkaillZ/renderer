#pragma once

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/quaternion.hpp>

#include "Model.hpp"
#include "VulkanDevice.hpp"
#include "Pipeline.hpp"
#include "PipelineSettings.hpp"

class ModelLoader {
    
public:
    static std::shared_ptr<Model> fromFile(std::string path, VulkanDevice& device, std::shared_ptr<PipelineSettings> pipelineSettings, std::shared_ptr<Uniforms<LocalTransform>> uniforms, std::string rootName = "");
    
private:
    static std::vector<std::shared_ptr<Mesh>> loadMeshes(const aiScene *scene, VulkanDevice& device);
    static std::unordered_map<std::string, MeshBoneData> loadMeshBoneData(aiMesh* mesh, std::vector<Vertex>& vertices);
    static std::unique_ptr<Skeleton> loadSkeleton(const aiScene* scene, std::string& rootName);
    static std::unordered_map<std::string, Animation> loadAnimations(const aiScene* scene);
    
    static void processMeshNodes(aiNode* node, const aiScene *scene, std::vector<aiMesh*>& meshes);
    static std::shared_ptr<Bone> processBoneNodes(aiNode* node, std::shared_ptr<Bone> parent);
    static aiNode* findRootNode(std::string rootName, aiNode* node);
    
    static glm::mat4 convertMatrix(aiMatrix4x4 m);
    
};
