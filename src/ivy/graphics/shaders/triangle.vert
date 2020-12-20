#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out VertexData {
    vec3 color;
    vec3 worldPos;
} VS_OUT;

void main() {
    vec4 pos = vec4(inPosition, 1.0);
    gl_Position = pos;

    VS_OUT.color = inColor;
    VS_OUT.worldPos = pos.xyz;
}
