#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

#include "consts.glsl"

const uint LIGHT_DIRECTIONAL = 0;
const uint LIGHT_POINT       = 1;

Shadow getShadow(Light light, vec3 surface_position);
float getShadowTerm(Light light, vec3 surface_position);
vec3 getToLightVector(Light light);

// Calculate illuminance for a light and surface
vec3 getIlluminance(Light light, vec3 surface_normal, vec3 surface_position) {
    float attenuation;
    switch (light.lightType) {
        case LIGHT_DIRECTIONAL:
            // cos(theta) attenuation
            attenuation = dot(surface_normal, -normalize(light.directionAndShadowBias.xyz));
            break;
        case LIGHT_POINT:
            vec3  toLight  = light.directionAndShadowBias.xyz - surface_position;
            float distance = length(toLight);

            // cos(theta) attenuation
            attenuation = dot(surface_normal, toLight / distance);

            // Inverse square law
            // the 4*PI is for converting luminous power (lumens) to luminous intensity (lux)
            attenuation /= 4 * PI * (distance * distance);
            break;
    }

    vec3  color         = light.colorAndIntensity.rgb;
    float luminousPower = light.colorAndIntensity.a;

    return color * luminousPower * max(0, attenuation) * getShadowTerm(light, surface_position);
}

// Helper funcs

Shadow getShadow(Light light, vec3 surface_position) {
    Shadow shadow;
    switch (light.lightType) {
        case LIGHT_DIRECTIONAL:
        // Light space position
        vec4 positionLS = light.viewProjection * vec4(surface_position, 1);

        // To UV
        vec3 positionUV = positionLS.xyz / positionLS.w;
        positionUV.xy   = positionUV.xy * 0.5 + 0.5;

        // Remap positionUV to shadow viewport
        vec2 uv = positionUV.xy * light.shadowViewportNormalized.zw + light.shadowViewportNormalized.xy;

        // Directional light shadow map depth is not linear
        shadow.shadowDepth  = texture(uShadowMapDirectional, uv).r;
        shadow.currentDepth = positionUV.z;
        break;
        case LIGHT_POINT:
        uint  shadowIndex = light.shadowIndex;
        float nearPlane   = light.shadowViewportNormalized.x;
        float farPlane    = light.shadowViewportNormalized.y;

        vec3 lightDir = surface_position - light.directionAndShadowBias.xyz;

        // Point light shadow map stores linear depth
        shadow.shadowDepth = mix(nearPlane, farPlane, texture(uShadowMapPoint, vec4(lightDir, shadowIndex)).r);
        shadow.currentDepth = length(lightDir);
        break;
    }
    return shadow;
}

float getShadowTerm(Light light, vec3 surface_position) {
    Shadow shadow     = getShadow(light, surface_position);
    float  shadowBias = light.directionAndShadowBias.w;

    return step(shadow.currentDepth - shadowBias, shadow.shadowDepth);
}

vec3 getToLightVector(Light light, vec3 surface_position) {
    switch (light.lightType) {
        case LIGHT_DIRECTIONAL:
            return normalize(light.directionAndShadowBias.xyz);
        case LIGHT_POINT:
            return normalize(light.directionAndShadowBias.xyz - surface_position);
    }

    return vec3(0);
}

#endif // LIGHTS_GLSL
