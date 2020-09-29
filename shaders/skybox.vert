#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform LocalTransform {
    mat4 model;
    mat4 view;
    mat4 proj;
} t;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragTexCoord;

void main() {
    fragTexCoord = inPosition;
    
    // mat4(mat3()) removes translation and scale
    vec4 pos = t.proj * mat4(mat3(t.view)) * mat4(mat3(t.model)) * vec4(inPosition, 1.0); // Always in the center
    gl_Position = pos.xyww;
}
