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
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inTangent;
layout(location = 5) in uvec4 inBoneIds;
layout(location = 6) in vec4 inBoneWeights;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out mat3 fragTbn;
layout(location = 7) out vec4 fragShadowCoord;

void main() {
    mat4 boneTransform = t.boneTransforms[inBoneIds[0]] * inBoneWeights[0];
    boneTransform += t.boneTransforms[inBoneIds[1]] * inBoneWeights[1];
    boneTransform += t.boneTransforms[inBoneIds[2]] * inBoneWeights[2];
    boneTransform += t.boneTransforms[inBoneIds[3]] * inBoneWeights[3];
    
    fragPos = vec3(t.model * boneTransform * vec4(inPosition, 1.0));
    fragColor = inColor;
    fragNormal = mat3(transpose(inverse(t.model))) * inNormal;
    fragTexCoord = inTexCoord;
 
    vec3 T = normalize(vec3(t.model * boneTransform * vec4(inTangent.xyz, 0.0)));
    vec3 N = normalize(vec3(t.model * boneTransform * vec4(inNormal, 0.0)));
    
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    
    vec3 B = cross(T, N) * inTangent.w;
    fragTbn = mat3(T, B, N);
    
    fragShadowCoord = t.lightSpace * vec4(fragPos, 1.0);
    
    gl_Position = t.proj * t.view * vec4(fragPos, 1.0);
}
