#include "ModelLoader.hpp"

std::shared_ptr<Model> ModelLoader::fromFile(std::string path, VulkanDevice& device, std::shared_ptr<PipelineSettings> pipelineSettings, std::shared_ptr<Uniforms<LocalTransform>> uniforms, std::string skeletonRoot) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights | aiProcess_GenSmoothNormals);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error(std::string("Assimp Error: ") + importer.GetErrorString());
    }
    
    auto meshes = loadMeshes(scene, device);
    auto animations = loadAnimations(scene);
    auto skeleton = loadSkeleton(scene, skeletonRoot);
    skeleton->getRoot()->print();
    return std::make_shared<Model>(std::move(meshes), std::move(animations), pipelineSettings, uniforms, std::move(skeleton), device);
}

std::vector<std::shared_ptr<Mesh>> ModelLoader::loadMeshes(const aiScene *scene, VulkanDevice& device) {
    std::vector<aiMesh*> aiMeshes;
    processMeshNodes(scene->mRootNode, scene, aiMeshes);
    if (!aiMeshes.size()) {
        throw std::runtime_error("No meshes found in model.");
    }
    
    std::vector<std::shared_ptr<Mesh>> meshes;
    for (const auto aiMesh : aiMeshes) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        for (uint32_t i = 0; i < aiMesh->mNumVertices; i++)
        {
            Vertex vertex = {};
            
            vertex.pos = {
                aiMesh->mVertices[i].x,
                aiMesh->mVertices[i].y,
                aiMesh->mVertices[i].z
            };
            
            vertex.normal = {
                aiMesh->mNormals[i].x,
                aiMesh->mNormals[i].y,
                aiMesh->mNormals[i].z
            };
            
            if (aiMesh->HasTangentsAndBitangents()) {
                glm::vec3 tangent = {
                    aiMesh->mTangents[i].x,
                    aiMesh->mTangents[i].y,
                    aiMesh->mTangents[i].z
                };
                
                glm::vec3 bitangent = {
                    aiMesh->mBitangents[i].x,
                    aiMesh->mBitangents[i].y,
                    aiMesh->mBitangents[i].z
                };
                
                // Calculate w component
                glm::vec3 c = glm::cross(vertex.normal, tangent);
                float w = (glm::dot(c, bitangent) < 0) ? 1.0f : -1.0f;
                
                vertex.tangent = {
                    tangent.x,
                    tangent.y,
                    tangent.z,
                    w
                };
            }
            
            if (aiMesh->HasVertexColors(0)) {
                vertex.color = {
                    aiMesh->mColors[0][i].r,
                    aiMesh->mColors[0][i].g,
                    aiMesh->mColors[0][i].b,
                    aiMesh->mColors[0][i].a,
                };
            } else {
                vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
            }
            
            if (aiMesh->HasTextureCoords(0)) {
                vertex.texCoord = {
                    aiMesh->mTextureCoords[0][i].x,
                    aiMesh->mTextureCoords[0][i].y,
                };
            } else {
                vertex.texCoord = {};
            }
            
            vertices.push_back(vertex);
        }
        
        for (uint32_t i = 0; i < aiMesh->mNumFaces; i++)
        {
            auto face = aiMesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }
        
        auto meshBoneData = loadMeshBoneData(aiMesh, vertices);
        meshes.push_back(std::make_shared<Mesh>(device, vertices, indices, meshBoneData));
    }
    
    return meshes;
}

void ModelLoader::processMeshNodes(aiNode* node, const aiScene *scene, std::vector<aiMesh*>& meshes) {
    // process all the node's meshes (if any)
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        meshes.push_back(scene->mMeshes[node->mMeshes[i]]);
    }
    // then do the same for each of its children
    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        processMeshNodes(node->mChildren[i], scene, meshes);
    }
}

std::unordered_map<std::string, MeshBoneData> ModelLoader::loadMeshBoneData(aiMesh* mesh, std::vector<Vertex>& vertices) {
    std::unordered_map<std::string, MeshBoneData> bones;
    
    // Key: vertex index, value: number of bone weights
    std::unordered_map<uint32_t, uint32_t> boneWeightsPerVertex;
    
    for (uint32_t j = 0; j < mesh->mNumBones; j++) {
        auto bone = mesh->mBones[j];
        
        std::string boneName(bone->mName.C_Str());
        bones[boneName] = MeshBoneData(boneName, j, convertMatrix(bone->mOffsetMatrix));
        
        for (uint32_t k = 0; k < bone->mNumWeights; k++) {
            auto weight = bone->mWeights[k];
            
            int weightIndex = boneWeightsPerVertex.count(weight.mVertexId) > 0
                ? boneWeightsPerVertex[weight.mVertexId] : 0;
            
            if (weightIndex > Vertex::BONES_PER_VERTEX - 1) {
                throw std::runtime_error("Too many bone weights per vertex!");
            }
            
            vertices[weight.mVertexId].boneWeights[weightIndex] = weight.mWeight;
            vertices[weight.mVertexId].boneIds[weightIndex] = j;
            
            boneWeightsPerVertex[weight.mVertexId] = ++weightIndex;
        }
    }
    
    return bones;
}

std::unique_ptr<Skeleton> ModelLoader::loadSkeleton(const aiScene* scene, std::string& rootName) {
    aiNode* root = rootName != "" ? findRootNode(rootName, scene->mRootNode) : scene->mRootNode;
    auto rootBone = processBoneNodes(root, nullptr);
    return std::make_unique<Skeleton>(rootBone);
}

std::shared_ptr<Bone> ModelLoader::processBoneNodes(aiNode* node, std::shared_ptr<Bone> parent) {
    auto bone = std::make_shared<Bone>(std::string(node->mName.C_Str()), parent, convertMatrix(node->mTransformation));
    
    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        bone->children.push_back(processBoneNodes(node->mChildren[i], bone));
    }
    
    return bone;
}

aiNode* ModelLoader::findRootNode(std::string rootName, aiNode* node) {
    if (strcmp(node->mName.C_Str(), rootName.c_str()) == 0) {
        return node;
    }
    
    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        aiNode* n = findRootNode(rootName, node->mChildren[i]);
        if (n != nullptr) {
            return n;
        }
    }
    
    return nullptr;
}

std::unordered_map<std::string, Animation> ModelLoader::loadAnimations(const aiScene* scene) {
    std::unordered_map<std::string, Animation> animations;
    
    for (uint32_t i = 0; i < scene->mNumAnimations; i++) {
        auto aiAnimation = scene->mAnimations[i];
        
        auto animation = Animation(std::string(aiAnimation->mName.C_Str()), static_cast<float>(aiAnimation->mDuration), static_cast<float>(aiAnimation->mTicksPerSecond));
        
        for (uint32_t j = 0; j < aiAnimation->mNumChannels; j++) {
            auto aiChannel = aiAnimation->mChannels[j];
            
            auto& channel = animation.createChannel(std::string(aiChannel->mNodeName.C_Str()));
            
            for (uint32_t k = 0; k < aiChannel->mNumPositionKeys; k++) {
                auto key = aiChannel->mPositionKeys[k];
                channel.addTranslationKey(key.mTime, glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z));
            }
            
            for (uint32_t k = 0; k < aiChannel->mNumRotationKeys; k++) {
                auto key = aiChannel->mRotationKeys[k];
                channel.addRotationKey(key.mTime, glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z));
            }
            
            for (uint32_t k = 0; k < aiChannel->mNumScalingKeys; k++) {
                auto key = aiChannel->mScalingKeys[k];
                channel.addScaleKey(key.mTime, glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z));
            }
        }
        
        animations.emplace(animation.getName(), animation);
    }
    
    return animations;
}

glm::mat4 ModelLoader::convertMatrix(aiMatrix4x4 m) {
    // GLM is column-major!
    return glm::mat4 {
        { m.a1, m.b1, m.c1, m.d1 },
        { m.a2, m.b2, m.c2, m.d2 },
        { m.a3, m.b3, m.c3, m.d3 },
        { m.a4, m.b4, m.c4, m.d4 }
    };
}
