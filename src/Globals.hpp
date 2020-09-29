#pragma once

#include "Light.hpp"

const int MAX_BONES = 64;

struct LocalTransform {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 lightSpace;
    alignas(16) glm::mat4 boneTransforms[MAX_BONES];
};

struct LocalTransformShadow {
    alignas(16) glm::mat4 depthMvp;
};

struct Globals {
    alignas(16) glm::vec3 viewPos;
    alignas(16) glm::vec3 ambientColor;
    alignas(16) Light lights[MAX_LIGHTS];
};
