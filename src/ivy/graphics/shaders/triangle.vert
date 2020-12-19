#version 450

// TODO: these are unused currently
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out VertexData {
    vec3 color;
    vec3 worldPos;
} VS_OUT;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    vec4 pos = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    gl_Position = pos;

    VS_OUT.color = inColor;
    VS_OUT.worldPos = pos.xyz;
}
