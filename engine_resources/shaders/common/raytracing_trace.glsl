// Traversal layer of the shared BVH ray-tracing code - see raytracing_types.glsl for the struct types
// these operate on and the required include order (this file must come AFTER the including shader
// declares its own bvhNodes[]/triPos[] SSBOs, since these functions reference those global array names
// directly rather than taking buffers as parameters).

bool IntersectAABB(vec3 ro, vec3 invRd, vec3 boundsMin, vec3 boundsMax, float maxT)
{
    vec3 t0 = (boundsMin - ro) * invRd;
    vec3 t1 = (boundsMax - ro) * invRd;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);
    return tFar >= max(tNear, 0.0) && tNear < maxT;
}

// Moller-Trumbore. Shrinks `closestT` (the caller's current best hit distance) in place so the BVH
// walk below can use it to reject farther AABBs/triangles as it goes.
bool IntersectTriangle(vec3 ro, vec3 rd, uint triIdx, inout float closestT, out vec2 outBary)
{
    TrianglePos tri = triPos[triIdx];
    vec3 e1 = tri.v1.xyz - tri.v0.xyz;
    vec3 e2 = tri.v2.xyz - tri.v0.xyz;
    vec3 p = cross(rd, e2);
    float det = dot(e1, p);
    if (abs(det) < 1e-8)
        return false;

    float invDet = 1.0 / det;
    vec3 tvec = ro - tri.v0.xyz;
    float u = dot(tvec, p) * invDet;
    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q = cross(tvec, e1);
    float v = dot(rd, q) * invDet;
    if (v < 0.0 || u + v > 1.0)
        return false;

    float t = dot(e2, q) * invDet;
    if (t < 1e-4 || t > closestT)
        return false;

    closestT = t;
    outBary = vec2(u, v);
    return true;
}

// Same Moller-Trumbore test as IntersectTriangle, but for shadow/occlusion rays that only need to know
// IF something blocks the ray, not the closest hit or its barycentrics.
bool IntersectTriangleAny(vec3 ro, vec3 rd, uint triIdx, float maxT)
{
    TrianglePos tri = triPos[triIdx];
    vec3 e1 = tri.v1.xyz - tri.v0.xyz;
    vec3 e2 = tri.v2.xyz - tri.v0.xyz;
    vec3 p = cross(rd, e2);
    float det = dot(e1, p);
    if (abs(det) < 1e-8)
        return false;

    float invDet = 1.0 / det;
    vec3 tvec = ro - tri.v0.xyz;
    float u = dot(tvec, p) * invDet;
    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q = cross(tvec, e1);
    float v = dot(rd, q) * invDet;
    if (v < 0.0 || u + v > 1.0)
        return false;

    float t = dot(e2, q) * invDet;
    return t > 1e-4 && t < maxT;
}

// Any-hit BVH traversal for shadow rays : returns as soon as a single occluder is found instead of
// searching for the closest one - roughly halves the ray-tracing cost of NEE compared to reusing the
// closest-hit TraceRay for occlusion tests too.
bool TraceRayAny(vec3 ro, vec3 rd, float maxT)
{
    vec3 invRd = 1.0 / rd;

    int stack[64];
    int sp = 0;
    stack[sp++] = 0;

    int iterations = 0;
    while (sp > 0 && iterations < 4096)
    {
        iterations++;

        BVHNode node = bvhNodes[stack[--sp]];

        if (!IntersectAABB(ro, invRd, node.boundsMin, node.boundsMax, maxT))
            continue;

        if (node.triCount > 0u)
        {
            for (uint i = 0u; i < node.triCount; i++)
            {
                if (IntersectTriangleAny(ro, rd, node.leftFirst + i, maxT))
                    return true;
            }
        }
        else if (sp < 62)
        {
            stack[sp++] = int(node.leftFirst);
            stack[sp++] = int(node.leftFirst) + 1;
        }
    }

    return false;
}

// Iterative BVH traversal (GLSL has no recursion) - 64 stack slots comfortably covers even
// million-triangle scenes for a well-balanced tree (depth ~log2(N)), but a pathologically unbalanced
// BVH (or corrupted node data) could in principle push past that. Writing past the end of a local array
// in GLSL is undefined behavior - not a clean crash - and has been observed to manifest as an
// effectively endless loop (garbage `sp`/node data keeps the loop going) that hangs the driver instead
// of failing safely. Both the stack-depth check on push and the hard `iterations` cap below exist
// purely as a safety net against that : a correct traversal never gets close to either limit.
bool TraceRay(vec3 ro, vec3 rd, float maxT, out HitInfo hit)
{
    hit.t = maxT;
    hit.triIndex = 0xFFFFFFFFu;

    vec3 invRd = 1.0 / rd;

    int stack[64];
    int sp = 0;
    stack[sp++] = 0;

    int iterations = 0;
    while (sp > 0 && iterations < 4096)
    {
        iterations++;

        BVHNode node = bvhNodes[stack[--sp]];

        if (!IntersectAABB(ro, invRd, node.boundsMin, node.boundsMax, hit.t))
            continue;

        if (node.triCount > 0u)
        {
            for (uint i = 0u; i < node.triCount; i++)
            {
                uint triIdx = node.leftFirst + i;
                vec2 bary;
                if (IntersectTriangle(ro, rd, triIdx, hit.t, bary))
                {
                    hit.triIndex = triIdx;
                    hit.bary = bary;
                }
            }
        }
        else if (sp < 62)
        {
            stack[sp++] = int(node.leftFirst);
            stack[sp++] = int(node.leftFirst) + 1;
        }
    }

    return hit.triIndex != 0xFFFFFFFFu;
}
