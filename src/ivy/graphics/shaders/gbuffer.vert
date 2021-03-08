#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (set = 0, binding = 0) uniform MVP {
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 normal;
} uMVP;

layout (location = 0) out VertexData {
    vec3 normal;
    vec2 uv;
} VS_OUT;

void main() {
    gl_Position = uMVP.projection * uMVP.view * uMVP.model * vec4(inPosition, 1.0);

    VS_OUT.normal = vec3(uMVP.normal * vec4(inNormal, 1));
    VS_OUT.uv = inUV;
}
