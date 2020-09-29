#include "Animation.hpp"

Animation::Animation(std::string name, float duration, float ticksPerSecond)
    : name(name), duration(duration), ticksPerSecond(ticksPerSecond), boneAnimationChannels() {}

AnimationChannel& Animation::createChannel(std::string boneName) {
    boneAnimationChannels[boneName] = AnimationChannel(boneName);
    return boneAnimationChannels[boneName];
}

inline AnimationChannel& Animation::getChannel(std::string boneName) {
    if (!hasChannel(boneName)) {
        throw std::runtime_error("No animation channel named '" + boneName + "' exists.");
    }
    
    return boneAnimationChannels[boneName];
}

inline bool Animation::hasChannel(std::string boneName) {
    return boneAnimationChannels.count(boneName) != 0;
}

void Animation::evaluate(Mesh& mesh, Skeleton& skeleton, float time) {
    float timeInTicks = time * ticksPerSecond;
    float animationTime = fmod(timeInTicks, duration);
    traverseBoneHierarchy(animationTime, skeleton.getRoot(), glm::mat4(1.0f), mesh.getBoneData(), mesh.getBoneTransforms());
}

glm::vec3 Animation::calculateTranslation(AnimationChannel& channel, float animationTime) {
    if (channel.translationKeys.size() == 0) {
        return glm::vec3();
    }
    
    if (channel.translationKeys.size() == 1) {
        return channel.translationKeys[0].value;
    }
    
    auto currentIndex = channel.findTranslationIndex(animationTime);
    auto nextIndex = (currentIndex + 1) % channel.translationKeys.size();
    
    auto& currentKey = channel.translationKeys[currentIndex];
    auto& nextKey = channel.translationKeys[nextIndex];
    
    float deltaTime = nextKey.time - currentKey.time;
    float relativeTime = (animationTime - currentKey.time) / deltaTime;
    relativeTime = glm::clamp(relativeTime, 0.0f, 1.0f);
    
    return glm::mix(currentKey.value, nextKey.value, relativeTime);
}

glm::quat Animation::calculateRotation(AnimationChannel& channel, float animationTime) {
    if (channel.rotationKeys.size() == 0) {
        return glm::quat();
    }
    
    if (channel.rotationKeys.size() == 1) {
        return channel.rotationKeys[0].value;
    }
    
    auto currentIndex = channel.findRotationIndex(animationTime);
    auto nextIndex = (currentIndex + 1) % channel.rotationKeys.size();
    
    auto& currentKey = channel.rotationKeys[currentIndex];
    auto& nextKey = channel.rotationKeys[nextIndex];
    
    float deltaTime = nextKey.time - currentKey.time;
    float relativeTime = (animationTime - currentKey.time) / deltaTime;
    relativeTime = glm::clamp(relativeTime, 0.0f, 1.0f);
    
    return glm::mix(currentKey.value, nextKey.value, relativeTime);
}

glm::vec3 Animation::calculateScale(AnimationChannel& channel, float animationTime) {
    if (channel.scaleKeys.size() == 0) {
        return glm::vec3();
    }
    
    if (channel.scaleKeys.size() == 1) {
        return channel.scaleKeys[0].value;
    }
    
    auto currentIndex = channel.findScaleIndex(animationTime);
    auto nextIndex = (currentIndex + 1) % channel.scaleKeys.size();
    
    auto& currentKey = channel.scaleKeys[currentIndex];
    auto& nextKey = channel.scaleKeys[nextIndex];
    
    float deltaTime = nextKey.time - currentKey.time;
    float relativeTime = (animationTime - currentKey.time) / deltaTime;
    relativeTime = glm::clamp(relativeTime, 0.0f, 1.0f);
    
    return glm::mix(currentKey.value, nextKey.value, relativeTime);
}

void Animation::traverseBoneHierarchy(float animationTime, std::shared_ptr<Bone> bone, glm::mat4 parentTransform, std::unordered_map<std::string, MeshBoneData>& boneData, std::vector<glm::mat4>& boneTransforms)
{
    glm::mat4 boneTransformation = bone->transformation;
    std::string& boneName = bone->name;
    
    if (hasChannel(boneName))
    {
        auto& animationChannel = getChannel(boneName);
        
        auto scale = calculateScale(animationChannel, animationTime);
        auto rotation = calculateRotation(animationChannel, animationTime);
        auto translation = calculateTranslation(animationChannel, animationTime);
        
        auto translationMatrix = glm::translate(glm::mat4(1.0f), translation);
        auto rotationMatrix = glm::toMat4(rotation);
        auto scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        
        boneTransformation = translationMatrix * rotationMatrix * scaleMatrix;
    }
    
    glm::mat4 globalTransformation = parentTransform * boneTransformation;
    
    if (boneData.count(boneName) != 0) {
        boneTransforms[boneData[boneName].index] = globalTransformation * boneData[boneName].offset;
    }
    
    for (const auto& child : bone->children) {
        traverseBoneHierarchy(animationTime, child, globalTransformation, boneData, boneTransforms);
    }
}
