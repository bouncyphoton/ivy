#version 450

layout (location = 0) out VertexData {
    vec2 uv;
} VS_OUT;

void main() {
    /* The following line of code is the same as...

        vec2 uvs[3] = vec2[3](
            vec2(0, 0),
            vec2(2, 0),
            vec2(0, 2)
        );

        vec4 vertices[3] = vec4[3](
            vec4(-1.0, -1.0,  0.0, 1),
            vec4( 3.0, -1.0,  0.0, 1),
            vec4(-1.0,  3.0,  0.0, 1)
        );

        VS_OUT.uv = uvs[gl_VertexIndex];
        gl_Position = vertices[gl_VertexIndex];
    */

    VS_OUT.uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(VS_OUT.uv * 2.0f - 1.0f, 0.0f, 1.0f);
}
