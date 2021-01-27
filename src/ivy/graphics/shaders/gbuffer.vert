#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout (set = 0, binding = 0) uniform MVP {
    mat4 projection;
    mat4 view;
    mat4 model;
} uMVP;

layout (location = 0) out VertexData {
    vec3 color;
    vec3 worldPos;
} VS_OUT;

void main() {
    vec4 pos = uMVP.projection * uMVP.view * uMVP.model * vec4(inPosition, 1.0);
    gl_Position = pos;

    VS_OUT.color = inColor;
    VS_OUT.worldPos = pos.xyz;
}
