#version 450
#include "structs.glsl"

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput iDiffuse;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput iNormal;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput iOcclusionRoughnessMetallic;
layout (input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput iDepth;

layout (location = 0) in VertexData {
    vec2 uv;
} FS_IN;

layout (location = 0) out vec4 oFragColor;

layout (set = 1, binding = 0) uniform PerFrame {
    mat4 invProjection;
    mat4 invView;
    vec2 resolution;
    uint debugMode;
} uFrame;

layout (set = 1, binding = 1) uniform sampler2D uShadowMapDirectional;
layout (set = 1, binding = 2) uniform samplerCubeArray uShadowMapPoint;

layout (set = 2, binding = 0) uniform PerLight { Light light; } uLight;

#include "utils.glsl"   // depthToWorldPos
#include "lights.glsl"  // Light, getIlluminance
#include "brdf.glsl"    // brdf
#include "tonemap.glsl" // ACESFilm, srgbToLinear, linearToSrgb

const uint DEBUG_FULL       = 0;
const uint DEBUG_DIFFUSE    = 1;
const uint DEBUG_NORMAL     = 2;
const uint DEBUG_ORM        = 3;
const uint DEBUG_SHADOW_MAP = 4;

void main() {
    // Given variables
    float depth = subpassLoad(iDepth).r;
    if (depth == 1) {
        discard;
    }
    vec3 diffuse = srgbToLinear(subpassLoad(iDiffuse).rgb);
    vec3 normal = normalize(subpassLoad(iNormal).xyz);
    float occlusion = subpassLoad(iOcclusionRoughnessMetallic).r;
    float roughness = subpassLoad(iOcclusionRoughnessMetallic).g;
    float metallic = subpassLoad(iOcclusionRoughnessMetallic).b;

    // Derived variables
    vec2 uv       = FS_IN.uv;
    vec3 worldPos = depthToWorldPos(uv, depth, uFrame.invProjection, uFrame.invView);
    vec3 viewPos  = uFrame.invView[3].xyz;
    vec3 viewDir  = normalize(worldPos - viewPos);

    vec3 lightDirection = getToLightVector(uLight.light, worldPos);

    vec3 color = vec3(0);
    switch (uFrame.debugMode) {
        case DEBUG_FULL:
            color = brdf(diffuse, occlusion, roughness, metallic, normal, -viewDir, lightDirection) * getIlluminance(uLight.light, normal, worldPos);
            break;
        case DEBUG_DIFFUSE:
            color = diffuse;
            break;
        case DEBUG_NORMAL:
            color = normal * 0.5 + 0.5;
            break;
        case DEBUG_ORM:
            color = vec3(occlusion, roughness, metallic);
            break;
        case DEBUG_SHADOW_MAP:
            color = texture(uShadowMapPoint, vec4(lightDirection, uLight.light.shadowIndex)).rrr;
            break;
    }

    // Tonemap and gamma correct
    color = ACESFilm(color);
    color = linearToSrgb(color);

    oFragColor = vec4(color, 1);
}
