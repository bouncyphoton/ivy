#version 450

layout (location = 0) in VertexData {
    vec3 color;
    vec3 worldPos;
} FS_IN;

layout (location = 0) out vec4 oAlbedo;
layout (location = 1) out vec4 oPosition;

void main() {
    oAlbedo = vec4(FS_IN.color, 1.0);
    oPosition = vec4(FS_IN.worldPos, 1.0);
}
