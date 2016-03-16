#version 450 core

out vec4 outColor;

const uint LONGEST = 0xFFFFFFFF;
const float INFINITY = 1e10;

struct Thash {
	uint triangle;
	uint previous;
};

struct Voxel {
	uint last;
    uint count;
    uint coordX;
	uint coordY;
	uint coordZ;
};

layout(std430, binding=0) buffer s_voxels {Voxel voxels[];};
layout(std430, binding=1) buffer s_voxels_sub {uint voxels_subgrid[];};
layout(std430, binding=2) buffer s_thashes {Thash thashes[];};
layout(std430, binding=3) buffer s_vbo {float verts[];};
layout(std430, binding=4) buffer s_ebo {uint indics[];};
layout(std430, binding=5) buffer s_norm {float norms[];};
layout (binding=0) uniform atomic_uint vcounter;

uniform mat4 projInv;
uniform mat4 camInv;
uniform vec2 sceneRes;
uniform uint maxDepth;
uniform vec3 offset;
uniform vec3 scale;

uint cnv_subgrid(uint sub, uvec3 idu){
    return sub * 8 + (idu.x + idu.y * 2 + idu.z * 4);
}

uint searchVoxelIndex(uvec3 idx){
    uint currentDepth = maxDepth-1;
    uvec3 id = uvec3(0);
    bool fail = false;
    uint sub = 0;
    Voxel hash = voxels[sub];
    uvec3 idu = uvec3(0);
    uint grid = 0;

    int strt = int(currentDepth);
    for(int i=strt;i>=0;i--){
        if(!fail){
            id = uvec3(idx / uint(pow(2, i)));
            idu = id % uvec3(2);
            if(i == strt){
                grid = 0;
                sub = 0;
            } else {
                grid = cnv_subgrid(sub, idu);
                sub = voxels_subgrid[grid];
            }
            if(sub == LONGEST) {
                fail = true;
            } else {
                hash = voxels[sub];
            }
        }
    }

    if(!fail){
        return sub;
    }
    return LONGEST;
}

Voxel loadVoxel(in uvec3 coord){
    if(coord.x != LONGEST){
        uint idx = searchVoxelIndex(coord);
        return voxels[idx];
    } else {
        Voxel voxel;
        voxel.last = LONGEST;
        voxel.count = 0;
        return voxel;
    }
}

vec2 intersectCube(vec3 origin, vec3 ray, vec3 cubeMin, vec3 cubeMax) {
   vec3 tMin = (cubeMin - origin) / ray;
   vec3 tMax = (cubeMax - origin) / ray;
   vec3 t1 = min(tMin, tMax);
   vec3 t2 = max(tMin, tMax);
   float tNear = max(max(t1.x, t1.y), t1.z);
   float tFar = min(min(t2.x, t2.y), t2.z);
   if( tNear <= tFar ){
       return vec2(tNear, tFar);
   } else {
       return vec2(INFINITY, INFINITY);
   }
}

float intersect(in vec3 orig, in vec3 dir, in vec3 ve[3], inout vec2 UV){
    vec3 e1 = ve[1] - ve[0];
    vec3 e2 = ve[2] - ve[0];
    vec3 normal = normalize(cross(e1, e2));
    float b = dot(normal, dir);
    vec3 w0 = orig - ve[0];
    float a = -dot(normal, w0);
    float t = a / b;
    vec3 p = orig + t * dir;
    float uu, uv, vv, wu, wv, inverseD;
    uu = dot(e1, e1);
    uv = dot(e1, e2);
    vv = dot(e2, e2);
    vec3 w = p - ve[0];
    wu = dot(w, e1);
    wv = dot(w, e2);
    inverseD = uv * uv - uu * vv;
    inverseD = 1.0f / inverseD;
    float u = (uv * wv - vv * wu) * inverseD;
    if (u < 0.0f || u > 1.0f)
    return -1.0f;
    float v = (uv * wu - uu * wv) * inverseD;
    if (v < 0.0f || (u + v) > 1.0f)
    return -1.0f;
    UV = vec2(u,v);
    return t;
}

struct TResult {
    float dist;
    uint triangle;
    vec3 normal;
};

TResult voxel_traversal(vec3 orig, vec3 dir, vec3 ray_start, vec3 ray_end) {
    float _bin_size = 1.0f;

    uint resX = uint(pow(2, maxDepth-1));

    vec3 p0 = ray_start / _bin_size;
    vec3 p1 = ray_end / _bin_size;
    vec3 ray = ray_end - ray_start;

    vec3 step = sign(p1 - p0);
    vec3 delta = _bin_size / abs(p1 - p0);
    vec3 test = floor(p0);
    vec3 mx = delta * mix(fract(p0), 1.0 - fract(p0), step * 0.5 + 0.5);

    TResult res;
    res.triangle = LONGEST;
    res.dist = INFINITY;

    float d = INFINITY;
    uint lastT = LONGEST;
    vec3 normal;

    const uint triangleCount = 1000;
    const uint iterationCount = uint(ceil(float(resX) * sqrt(3.0f))) * 2;
    uint iteration = 0;

    do {
        iteration++;

        uvec3 coord = uvec3(test);
        Voxel vox = loadVoxel(coord);

        if(vox.count > 0) {
            vec3 cmn = test + 0.0;
            vec3 cmx = test + 1.0;

            vec2 cd = intersectCube(ray_start, ray, cmn, cmx);
            float ncd = INFINITY;
            if(cd.x >= 0.0 && cd.y >= 0.0){
                ncd = min(cd.x, cd.y);
            } else
            if(cd.x >= 0.0 || cd.y >= 0.0){
                ncd = max(cd.x, cd.y);
            }

            if(ncd >= 0.0 && ncd < INFINITY){
                Thash hash = thashes[vox.last];
                bool needsSkip = false;

                float dd = INFINITY;
                uint lastTd = LONGEST;
                vec3 normald;
                for(uint i=0;i<=min(vox.count, triangleCount);i++){
                    if(!needsSkip){
                        uint tri = hash.triangle;

                        vec3 triverts[3];
                        uint ji[3];
                        for(uint x=0;x<3;x++){
                            uint j = indics[tri * 3 + x];
                            ji[x] = j;
                            triverts[x] = vec3(verts[j * 3 + 0], verts[j * 3 + 1], verts[j * 3 + 2]);
                        }

                        vec2 uv;
                        float _d = intersect(orig, dir, triverts, uv);
                        if(_d >= 0.0f && _d < dd) {
                            dd = _d;
                            lastTd = tri;

                            vec3 trinorms[3];
                            for(uint x=0;x<3;x++){
                                uint j = ji[x];
                                trinorms[x] = vec3(norms[j * 3 + 0], norms[j * 3 + 1], norms[j * 3 + 2]);
                            }

                            normald = normalize(trinorms[1]) * uv.x;
                            normald += normalize(trinorms[2]) * uv.y;
                            normald += normalize(trinorms[0]) * (1.0f - uv.x - uv.y);
                        }

                        uint previous = hash.previous;
                        if(previous != LONGEST){
                            hash = thashes[previous];
                        } else {
                            needsSkip = true;
                        }
                    }
                }

                if(dd > d && lastT != LONGEST && d >= 0.0f && d < INFINITY) {
                    res.dist = d;
                    res.triangle = lastT;
                    res.normal = normal;
                    return res;
                } else {
                    d = dd;
                    lastT = lastTd;
                    normal = normald;
                }
            }
        }

        if (mx.x < mx.y) {
            if (mx.x < mx.z) {
                test.x += step.x;
                mx.x += delta.x;
            } else {
                test.z += step.z;
                mx.z += delta.z;
            }
        } else {
            if (mx.y < mx.z) {
                test.y += step.y;
                mx.y += delta.y;
            } else {
                test.z += step.z;
                mx.z += delta.z;
            }
        }

    } while(
        iteration < iterationCount &&

        test.x >= 0.0f &&
        test.y >= 0.0f &&
        test.z >= 0.0f &&

        test.x < float(resX) &&
        test.y < float(resX) &&
        test.z < float(resX)
    );

    if(lastT != LONGEST && d >= 0.0f && d < INFINITY) {
        res.dist = d;
        res.triangle = lastT;
        res.normal = normal;
        return res;
    }

    return res;
}

TResult traverse(vec3 orig, vec3 dir){
    vec3 torig = orig;
    uint resX = uint(pow(2, maxDepth-1));
    torig -= offset;
    torig /= scale;
    torig *= resX;

    vec2 d = intersectCube(torig, dir, vec3(0.0f), vec3(resX));
    vec3 start = torig;
    vec3 end = torig + INFINITY * dir;
    if(d.y >= 0.0f && d.x < INFINITY && d.y < INFINITY){
        if(d.x >= 0.0f) start = (torig + dir * d.x);
        end = (torig + dir * d.y);
        return voxel_traversal(orig, dir, start, end);
    }

    TResult res;
    res.triangle = LONGEST;
    res.dist = INFINITY;
    return res;
}

void main()
{
    const vec3 light = vec3(0.0f, 3.0f, -9.0f);

    vec2 coord = floor(gl_FragCoord.xy - 0.5f + 0.0001f);
    coord /= sceneRes;
    coord = coord * 2.0f - 1.0f;

    vec4 co = camInv * projInv * vec4(coord, 0.0f, 1.0f);
    co /= co.w;
    vec4 ce = camInv * vec4(0.0f, 0.0f, 0.0f, 1.0f);

    vec3 orig = ce.xyz;
    vec3 dir = normalize(co.xyz - orig.xyz);
    vec3 color = vec3(0.0f);
    float rating = 1.0f;
    TResult res = traverse(orig, dir);
    for(int i=0;i<2;i++){
        if(res.triangle != LONGEST){
            orig += dir * res.dist;

            if(i == 0)
                rating = 1.0f;
            else
                rating = 0.25f;

            vec3 normal = normalize(res.normal * -dot(dir, res.normal));
            float light = dot(normalize(light - orig), normal) * 0.5 + 0.5;

            color = mix(color, vec3(0.9f, 0.5f, 0.5f) * light, rating);

            dir = reflect(dir, normal);
            orig += normalize(normal * dot(dir, normal)) * 0.0001f;

            res = traverse(orig, dir);
        }
    }

    outColor = vec4(color, 1.0f);
}