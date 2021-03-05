#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput iAlbedo;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput iNormal;

layout (location = 0) out vec4 oFragColor;

void main() {
    vec3 albedo = subpassLoad(iAlbedo).rgb;
    vec3 normal = normalize(subpassLoad(iNormal).xyz);

    vec3 color = albedo * (normal * 0.5 + 0.5);

    oFragColor = vec4(color, 1);
}
