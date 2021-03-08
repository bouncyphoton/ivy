#version 450
#include "utils.glsl"

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput iAlbedo;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput iNormal;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput iDepth;

layout (location = 0) out vec4 oFragColor;

layout (set = 1, binding = 0) uniform PerFrame {
    mat4 invProjection;
    mat4 invView;
    vec2 resolution;
} uFrame;

void main() {
    // Given variables
    float depth = subpassLoad(iDepth).r;
    if (depth == 1) {
        discard;
    }
    vec3 albedo = subpassLoad(iAlbedo).rgb;
    vec3 normal = normalize(subpassLoad(iNormal).xyz);

    // Derived variables
    vec2 uv       = vec2(gl_FragCoord.x, uFrame.resolution.y - gl_FragCoord.y) / uFrame.resolution;
    vec3 worldPos = depthToWorldPos(uv, depth, uFrame.invProjection, uFrame.invView);
    vec3 viewPos  = uFrame.invView[3].xyz;
    vec3 viewDir  = normalize(worldPos - viewPos);

    // TODO: remove temp hacks
    float ambient = 0.1f;
    vec3 sunDir = normalize(vec3(1));

    // Shade
    vec3 color = max(ambient, dot(normal, sunDir)) * albedo;
    oFragColor = vec4(color, 1);
}
