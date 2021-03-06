#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform LocalTransform {
    mat4 model;
    mat4 view;
    mat4 proj;
} t;

layout(binding = 2) uniform samplerCube albedoTex;

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(albedoTex, fragTexCoord);
}
