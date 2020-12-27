#version 450

vec4 vertices[3] = vec4[3](
    vec4(-1.0,  1.0,  1.0, 1),
    vec4( 2.0,  1.0,  1.0, 1),
    vec4(-1.0, -2.0,  1.0, 1)
);

void main() {
    gl_Position = vertices[gl_VertexIndex];
}
