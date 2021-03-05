#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (set = 0, binding = 0) uniform MVP {
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 normal;
} uMVP;

layout (location = 0) out VertexData {
    vec3 color;
    vec3 normal;
} VS_OUT;

void main() {
    gl_Position = uMVP.projection * uMVP.view * uMVP.model * vec4(inPosition, 1.0);

    VS_OUT.color = inColor;
    VS_OUT.normal = vec3(uMVP.normal * vec4(inNormal, 1));
}
