#version 450

layout (location = 0) in VertexData {
    mat3 tbn;
    vec2 uv;
} FS_IN;

layout (location = 0) out vec4 oDiffuse;
layout (location = 1) out vec4 oNormal;
layout (location = 2) out vec4 oOcclusionRoughnessMetallic;

layout (set = 1, binding = 0) uniform sampler2D uDiffuseTexture;
layout (set = 1, binding = 1) uniform sampler2D uNormalTexture;
layout (set = 1, binding = 2) uniform sampler2D uOcclusionTexture;
layout (set = 1, binding = 3) uniform sampler2D uRoughnessTexture;
layout (set = 1, binding = 4) uniform sampler2D uMetallicTexture;

void main() {
    oDiffuse = texture(uDiffuseTexture, FS_IN.uv);
    if (oDiffuse.a == 0) {
        discard;
    }

    oNormal.xyz = FS_IN.tbn * (texture(uNormalTexture, FS_IN.uv).rgb * 2 - 1);
    oNormal.w = 1;

    oOcclusionRoughnessMetallic.r = texture(uOcclusionTexture, FS_IN.uv).r;
    oOcclusionRoughnessMetallic.g = texture(uRoughnessTexture, FS_IN.uv).r;
    oOcclusionRoughnessMetallic.b = texture(uMetallicTexture, FS_IN.uv).r;
    oOcclusionRoughnessMetallic.a = 1;
}
