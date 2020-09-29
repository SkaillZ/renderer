#version 450
#extension GL_ARB_separate_shader_objects : enable

const int MAX_BONES = 64;

layout(binding = 0) uniform LocalTransform {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpace;
    mat4 boneTransforms[MAX_BONES];
} t;

layout(location = 0) in vec3 inPosition;
layout(location = 5) in uvec4 inBoneIds;
layout(location = 6) in vec4 inBoneWeights;

void main() {
    mat4 boneTransform = t.boneTransforms[inBoneIds[0]] * inBoneWeights[0];
    boneTransform += t.boneTransforms[inBoneIds[1]] * inBoneWeights[1];
    boneTransform += t.boneTransforms[inBoneIds[2]] * inBoneWeights[2];
    boneTransform += t.boneTransforms[inBoneIds[3]] * inBoneWeights[3];
    
    gl_Position = t.lightSpace * t.model * boneTransform * vec4(inPosition, 1.0);
}
