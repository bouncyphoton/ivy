#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout (set = 0, binding = 0) uniform PerLight {
    mat4 viewProjectionMatrices[6];
    vec3 lightPosition;
    vec2 nearAndFarPlanes;
    uint lightIndex;
} uPerLight;

layout (location = 0) out vec3 outPosition;

void main() {
    for (int face = 0; face < 6; ++face) {
        gl_Layer = 6 * int(uPerLight.lightIndex) + face;
        for (int v = 0; v < 3; ++v) {
            outPosition = gl_in[v].gl_Position.xyz;
            gl_Position = uPerLight.viewProjectionMatrices[face] * gl_in[v].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
