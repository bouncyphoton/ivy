#ifndef BRDF_GLSL
#define BRDF_GLSL

#include "consts.glsl"

// ggx distribution
float d_GGX(vec3 normal, vec3 half_vec, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(normal, half_vec), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom = a2;
    float den = (NdotH2 * (a2 - 1.0) + 1.0);
    den = PI * den * den;

    return nom / den;
}

// schlick fresnel
vec3 f_Schlick(float view_dot_half, vec3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - view_dot_half, 5.0);
}

// schlick ggx geometry
float g_Schlick_GGX(float normal_dot_view, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = normal_dot_view;
    float denom = normal_dot_view * (1.0 - k) + k;

    return nom / denom;
}

float g_Smith(vec3 normal, vec3 view, vec3 light, float roughness) {
    float NdotV = max(dot(normal, view), 0.0);
    float NdotL = max(dot(normal, light), 0.0);
    float ggx2 = g_Schlick_GGX(NdotV, roughness);
    float ggx1 = g_Schlick_GGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// lambertian diffuse + cook torrance microfacet
vec3 brdf(vec3 diffuse, float roughness, float metallic, vec3 normal, vec3 to_eye, vec3 to_light) {
    vec3  halfVec      = normalize(to_eye + to_light);
    float viewDotHalf = max(0, dot(to_eye, halfVec));

    vec3 f0 = mix(vec3(0.04), diffuse, metallic);

    float d = d_GGX(normal, halfVec, roughness);
    vec3  f = f_Schlick(viewDotHalf, f0);
    float g = g_Smith(normal, to_eye, to_light, roughness);

    vec3 Ks = (d * f * g) / max(EPSILON, (4 * max(0, dot(to_eye, normal)) * max(0, dot(to_light, normal))));

    vec3 Kd = (1 - Ks) * (1 - metallic) * diffuse / PI;

    return Kd + Ks;
}

#endif // BRDF_GLSL
