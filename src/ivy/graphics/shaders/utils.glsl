const float PI = 3.14159265358979;

vec3 depthToWorldPos(vec2 uv, float depth, mat4 invProjection, mat4 invView) {
    // To clip space
    vec4 positionCS = vec4(uv * 2 - 1, depth, 1.0f);

    // To view space
    vec4 positionVS = invProjection * positionCS;
    positionVS /= positionVS.w;

    // To world space
    vec4 positionWS = invView * positionVS;
    return positionWS.xyz;
}
