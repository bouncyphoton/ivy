#version 450

void main() {
    /* The following line of code is the same as...

        vec4 vertices[3] = vec4[3](
            vec4(-1.0, -1.0,  0.0, 1),
            vec4( 3.0, -1.0,  0.0, 1),
            vec4(-1.0,  3.0,  0.0, 1)
        );

        gl_Position = vertices[gl_VertexIndex];
    */

    gl_Position = vec4(vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f, 0.0f, 1.0f);
}
