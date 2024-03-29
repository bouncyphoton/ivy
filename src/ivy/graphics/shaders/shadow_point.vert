#version 450

layout (location = 0) in vec3 inPosition;
//layout (location = 1) in vec3 inNormal;
//layout (location = 2) in vec2 inUV;

layout (set = 1, binding = 0) uniform PerMesh {
    mat4 model;
} uPerMesh;

void main() {
    gl_Position = uPerMesh.model * vec4(inPosition, 1.0);
}
