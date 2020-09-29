#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Mesh.hpp"
#include "Skeleton.hpp"

template <typename T>
struct Keyframe {
    float time;
    T value;
    
    Keyframe(float time, T value) : time(time), value(value) {}
};

struct AnimationChannel {
    std::string name;
    
    std::vector<Keyframe<glm::vec3>> translationKeys;
    std::vector<Keyframe<glm::quat>> rotationKeys;
    std::vector<Keyframe<glm::vec3>> scaleKeys;
    
    void addTranslationKey(float time, glm::vec3 value) {
        translationKeys.push_back(Keyframe<glm::vec3>(time, value));
    }
    
    void addRotationKey(float time, glm::quat value) {
        rotationKeys.push_back(Keyframe<glm::quat>(time, value));
    }
    
    void addScaleKey(float time, glm::vec3 value) {
        scaleKeys.push_back(Keyframe<glm::vec3>(time, value));
    }
    
    AnimationChannel() : name("invalid") {}
    AnimationChannel(std::string name) : name(name) {}
    
    size_t findTranslationIndex(float time) {
        for (size_t i = 0; i < translationKeys.size(); i++) {
            if (time < translationKeys[i + 1].time) {
                return i;
            }
        }
        return 0;
    }
    
    size_t findRotationIndex(float time) {
        for (size_t i = 0; i < rotationKeys.size(); i++) {
            if (time < rotationKeys[i + 1].time) {
                return i;
            }
        }
        return 0;
    }
    
    size_t findScaleIndex(float time) {
        for (size_t i = 0; i < scaleKeys.size(); i++) {
            if (time < scaleKeys[i + 1].time) {
                return i;
            }
        }
        return 0;
    }
};

class Animation {
    
public:
    Animation() { throw std::runtime_error("Please call the other constructor."); } // Default constructor required for unordered_map
    Animation(std::string name, float duration, float ticksPerSecond);
    virtual ~Animation() {}
    
    void evaluate(Mesh& mesh, Skeleton& skeleton, float time);
    
    AnimationChannel& createChannel(std::string boneName);
    AnimationChannel& getChannel(std::string boneName);
    bool hasChannel(std::string boneName);
    
    std::string getName() { return name; }
    float getDuration() { return duration; }
    float getTicksPerSecond() { return ticksPerSecond; }
    
private:
    std::string name;
    float duration;
    float ticksPerSecond;
    
    std::unordered_map<std::string, AnimationChannel> boneAnimationChannels;
    
    glm::vec3 calculateTranslation(AnimationChannel& channel, float animationTime);
    glm::quat calculateRotation(AnimationChannel& channel, float animationTime);
    glm::vec3 calculateScale(AnimationChannel& channel, float animationTime);
    
    void traverseBoneHierarchy(float animationTime, std::shared_ptr<Bone> bone, glm::mat4 parentTransform, std::unordered_map<std::string, MeshBoneData>& boneData, std::vector<glm::mat4>& boneTransforms);
    
};
