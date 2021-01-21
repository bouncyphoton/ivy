#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput iAlbedo;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput iPosition;

layout (location = 0) out vec4 oFragColor;

void main() {
    oFragColor = subpassLoad(iAlbedo).rgba * subpassLoad(iPosition).rgba;
}
