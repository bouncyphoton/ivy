#ifndef STRUCTS_GLSL
#define STRUCTS_GLSL

struct Light {
    mat4 viewProjection;
    vec4 directionAndShadowBias;
    vec4 colorAndIntensity;
    vec4 shadowViewportNormalized;
    uint lightType;
    uint shadowIndex;
};

struct Shadow {
    float currentDepth;
    float shadowDepth;
};

#endif // STRUCTS_GLSL
