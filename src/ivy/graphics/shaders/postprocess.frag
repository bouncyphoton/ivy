#version 450

layout (location = 0) in VertexData {
    vec2 uv;
} FS_IN;

layout (location = 0) out vec4 oFragColor;

layout (set = 0, binding = 0) uniform sampler2D uTexture;

void main() {
    // TODO: tonemap here
    oFragColor = texture(uTexture, FS_IN.uv);
}
