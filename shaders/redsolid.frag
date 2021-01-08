#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform LocalTransform {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpace;
} t;

layout(binding = 1) uniform Globals {
    vec3 viewPos;
    vec3 ambientColor;
} globals;

layout(binding = 2) uniform sampler2D albedoTex;
layout(binding = 3) uniform sampler2D maskTex;
layout(binding = 4) uniform sampler2D normalMapTex;
layout(binding = 5) uniform sampler2D shadowMapTex;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in mat3 fragTbn;
layout(location = 7) in vec4 fragShadowCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0, 0, 1.0);
}
