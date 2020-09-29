#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform LocalTransform {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpace;
} t;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = t.lightSpace * t.model * vec4(inPosition, 1.0);
}
