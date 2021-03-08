#version 450

layout (location = 0) in VertexData {
    vec3 normal;
    vec2 uv;
} FS_IN;

layout (location = 0) out vec4 oDiffuse;
layout (location = 1) out vec4 oNormal;

layout (set = 1, binding = 0) uniform sampler2D uDiffuseTexture;

void main() {
    oDiffuse = texture(uDiffuseTexture, FS_IN.uv);
    if (oDiffuse.a == 0) {
        discard;
    }

    oNormal = vec4(FS_IN.normal, 1.0);
}
