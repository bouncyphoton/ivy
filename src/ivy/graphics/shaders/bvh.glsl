#ifndef BVH_GLSL
#define BVH_GLSL

#include "consts.glsl"

// This assumes the following is accessible
// VertexP3N3T3B3UV2
// uFrame.numIndices  -> number of indices
// uVertices.vertices -> variable length array of vertices
// uIndices.indices   -> variable length array of indices (of length numIndices)

struct HitInfo {
    float t;
    vec3 normal;
    // TODO: material
};

// Möller–Trumbore intersection algorithm
HitInfo intersectTriangle(vec3 ray_origin, vec3 ray_direction, VertexP3N3T3B3UV2 v0, VertexP3N3T3B3UV2 v1, VertexP3N3T3B3UV2 v2) {
    HitInfo isect;
    isect.t = -1;
    isect.normal = vec3(1, 0, 1);

    vec3 e1 = v1.position - v0.position;
    vec3 e2 = v2.position - v0.position;
    vec3 h = cross(ray_direction, e2);
    float a = dot(e1, h);

    // check if ray is parallel to the triangle
    if (a > -EPSILON && a < EPSILON) {
        return isect;
    }

    // calculate u
    float f = 1.0 / a;
    vec3 s = ray_origin - v0.position;
    float u = f * dot(s, h);

    // check if u is on triangle
    if (u < 0.0 || u > 1.0) {
        return isect;
    }

    // calculate v
    vec3 q = cross(s, e1);
    float v = f * dot(ray_direction, q);

    // check if v is on triangle
    if (v < 0.0 || u + v > 1.0) {
        return isect;
    }

    // get intersection
    float t = f * dot(e2, q);
    if (t > EPSILON) {
        isect.t = t;

        isect.normal = normalize((u * v0.normal) + (v * v1.normal) + ((1 - u - v) * v2.normal));
    }

    return isect;
}

// TODO: bvh traversal
HitInfo intersect(vec3 ray_origin, vec3 ray_direction) {
    HitInfo hit;
    hit.t = -1;
    hit.normal = vec3(1, 0, 0);
    bool intersected = false;

    for (uint i = 0; i < uFrame.numIndices; i += 3) {
        VertexP3N3T3B3UV2 v0 = getVertex(uIndices.indices[i + 0]);
        VertexP3N3T3B3UV2 v1 = getVertex(uIndices.indices[i + 1]);
        VertexP3N3T3B3UV2 v2 = getVertex(uIndices.indices[i + 2]);

        HitInfo latest = intersectTriangle(ray_origin, ray_direction, v0, v1, v2);
        if (latest.t != -1 && (latest.t < hit.t || !intersected)) {
            hit = latest;
            intersected = true;
        }
    }

    return hit;
}

#endif // BVH_GLSL
