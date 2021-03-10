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

layout (set = 1, binding = 1) uniform sampler2D uShadowMap;

layout (set = 2, binding = 0) uniform PerLight {
    mat4 viewProjection;
    vec4 directionAndShadowBias;
    vec4 colorAndIntensity;
    vec4 shadowViewportNormalized;
} uLight;

const uint DEBUG_FULL    = 0;
const uint DEBUG_DIFFUSE = 1;
const uint DEBUG_NORMAL  = 2;
const uint DEBUG_WORLD   = 3;
const uint DEBUG_SHADOW_MAP   = 4;

// returns (shadowMapDepth, currentDepth)
vec2 sampleShadowMap(vec3 positionWS) {
    // Light space position
    vec4 positionLS = uLight.viewProjection * vec4(positionWS, 1);

    // To UV
    vec3 positionUV = positionLS.xyz / positionLS.w;
    positionUV.xy   = positionUV.xy * 0.5 + 0.5;

    // Remap positionUV to shadow viewport
    vec2 uv = positionUV.xy * uLight.shadowViewportNormalized.zw + uLight.shadowViewportNormalized.xy;

    float shadowMapDepth = texture(uShadowMap, uv).r;
    float currentDepth   = positionUV.z;

    return vec2(shadowMapDepth, currentDepth);
}

float getDirectionalLightAttenuation(vec3 positionWS, vec3 normal) {
    vec3  direction  = uLight.directionAndShadowBias.xyz;
    float shadowBias = uLight.directionAndShadowBias.w;

    vec2  depths         = sampleShadowMap(positionWS);
    float shadowMapDepth = depths.x;
    float currentDepth   = depths.y;
    float shadowTerm     = currentDepth - shadowBias < shadowMapDepth ? 1 : 0;

    // TODO: pcf or something

    float attenuation = max(0, dot(normal, -normalize(direction)));

    return attenuation * shadowTerm;
}

float getAmbientFactor(vec3 positionWS) {
    float ambient = 0;
    const int halfChecks = 10;
    const float scale = 0.5;

    // Very fake, very quick GI
    for (int i = -halfChecks; i <= halfChecks; ++i) {
        for (int j = -halfChecks; j <= halfChecks; ++j) {
                if (i == 0 && j == 0) continue;

                vec2 depths = sampleShadowMap(positionWS + vec3(i, 0, j) * scale);
                if (abs(depths.x - depths.y) < 0.005) {
                    ambient += 1 / length(vec2(i * scale, j * scale));
                }
        }
    }

    return ambient / (halfChecks * halfChecks * 4) + 0.1;
}

void main() {
    // Given variables
    float depth = subpassLoad(iDepth).r;
    if (depth == 1) {
        discard;
    }
    vec3 diffuse = subpassLoad(iDiffuse).rgb;
    vec3 normal = normalize(subpassLoad(iNormal).xyz);

    vec3  lightDirection = uLight.directionAndShadowBias.xyz;
    vec3  lightColor     = uLight.colorAndIntensity.rgb;
    float lightIntensity = uLight.colorAndIntensity.a;

    // Derived variables
    vec2 uv       = vec2(gl_FragCoord.x, uFrame.resolution.y - gl_FragCoord.y) / uFrame.resolution;
    vec3 worldPos = depthToWorldPos(uv, depth, uFrame.invProjection, uFrame.invView);
    vec3 viewPos  = uFrame.invView[3].xyz;
    vec3 viewDir  = normalize(worldPos - viewPos);

    vec3 color = vec3(0);
    switch (uFrame.debugMode) {
        case DEBUG_FULL:
            // TODO: remove temp hacks
            float ambient   = getAmbientFactor(worldPos);
            float intensity = getDirectionalLightAttenuation(worldPos, normal) * lightIntensity;

            color = max(ambient, intensity) * lightColor * diffuse;
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
        case DEBUG_SHADOW_MAP:
            color = texture(uShadowMap, uv.xy).rrr;
            break;
    }

    oFragColor = vec4(color, 1);
}
