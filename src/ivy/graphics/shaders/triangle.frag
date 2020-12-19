#version 450

layout(location = 0) out vec4 oFragColor;

layout(location = 0) in VertexData {
    vec3 color;
    vec3 worldPos;
} FS_IN;

void main() {
    oFragColor = vec4(FS_IN.color, 1.0);
}
