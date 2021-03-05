#version 450

layout (location = 0) in VertexData {
    vec3 color;
    vec3 normal;
} FS_IN;

layout (location = 0) out vec4 oAlbedo;
layout (location = 1) out vec4 oNormal;

void main() {
    oAlbedo = vec4(FS_IN.color, 1.0);
    oNormal = vec4(FS_IN.normal, 1.0);
}
