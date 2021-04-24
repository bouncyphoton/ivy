#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inBiTangent;
layout (location = 4) in vec2 inUV;

layout (set = 0, binding = 0) uniform MVP {
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 normal;
} uMVP;

layout (location = 0) out VertexData {
    mat3 tbn;
    vec2 uv;
} VS_OUT;

void main() {
    gl_Position = uMVP.projection * uMVP.view * uMVP.model * vec4(inPosition, 1.0);

    vec3 bitangent = normalize(vec3(uMVP.normal * vec4(inBiTangent, 0)));
    vec3 tangent   = normalize(vec3(uMVP.normal * vec4(inTangent, 0)));
    vec3 normal    = normalize(vec3(uMVP.normal * vec4(inNormal, 0)));
    VS_OUT.tbn = mat3(tangent, bitangent, normal);
    VS_OUT.uv = inUV;
}
