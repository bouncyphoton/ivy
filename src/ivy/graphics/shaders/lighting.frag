#version 450
#include "utils.glsl"

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput iDiffuse;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput iNormal;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput iDepth;

layout (location = 0) out vec4 oFragColor;

layout (set = 1, binding = 0) uniform PerFrame {
    mat4 invProjection;
    mat4 invView;
    vec2 resolution;
    uint debugMode;
} uFrame;

const uint DEBUG_FULL    = 0;
const uint DEBUG_DIFFUSE = 1;
const uint DEBUG_NORMAL  = 2;
const uint DEBUG_WORLD   = 3;
const uint DEBUG_DEPTH   = 4;

void main() {
    // Given variables
    float depth = subpassLoad(iDepth).r;
    if (depth == 1) {
        discard;
    }
    vec3 diffuse = subpassLoad(iDiffuse).rgb;
    vec3 normal = normalize(subpassLoad(iNormal).xyz);

    // Derived variables
    vec2 uv       = vec2(gl_FragCoord.x, uFrame.resolution.y - gl_FragCoord.y) / uFrame.resolution;
    vec3 worldPos = depthToWorldPos(uv, depth, uFrame.invProjection, uFrame.invView);
    vec3 viewPos  = uFrame.invView[3].xyz;
    vec3 viewDir  = normalize(worldPos - viewPos);

    vec3 color = vec3(0);
    switch (uFrame.debugMode) {
        case DEBUG_FULL:
            // TODO: remove temp hacks
            float ambient = 0.1f;
            vec3 sunDir = normalize(vec3(1));
            color = max(ambient, dot(normal, sunDir)) * diffuse;
            break;
        case DEBUG_DIFFUSE:
            color = diffuse;
            break;
        case DEBUG_NORMAL:
            color = normal * 0.5 + 0.5;
            break;
        case DEBUG_WORLD:
            color = fract(worldPos);
            break;
        case DEBUG_DEPTH:
            color = vec3(depth);
            break;
    }

    oFragColor = vec4(color, 1);
}
