#version 450

layout (location = 0) in vec3 inPosition;

layout (set = 0, binding = 0) uniform PerLight {
    mat4 viewProjectionMatrices[6];
    vec3 lightPosition;
    vec2 nearAndFarPlanes;
    uint lightIndex;
} uPerLight;

void main() {
    float dist = length(inPosition - uPerLight.lightPosition);
    float near = uPerLight.nearAndFarPlanes.x;
    float far = uPerLight.nearAndFarPlanes.y;

    gl_FragDepth = (dist - near) / (far - near);
}
