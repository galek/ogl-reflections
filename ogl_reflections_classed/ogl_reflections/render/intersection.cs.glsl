#version 450 compatibility
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_shading_language_include : require
#include </constants>
#include </uniforms>
#include </fastmath>
#include </octree>
#include </structs>
#include </vertex_attributes>

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

float intersect(in vec3 orig, in vec3 dir, inout vec3 ve[3], inout vec2 UV)
{
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
    return INFINITY;
    float v = (uv * wu - uu * wv) * inverseD;
    if (v < 0.0f || (u + v) > 1.0f)
    return INFINITY;
    UV = vec2(u,v);
    return t;
}

struct TResult {
    float dist;
    uint triangle;
    vec3 normal;
    vec3 tangent;
    vec2 texcoord;
    int materialID;
};

TResult test_intersection(in vec3 orig, in vec3 dir, in Voxel vox){
    Thash hash = thashes[vox.last];
    bool needsSkip = false;

    const uint globalTCount = 300;
    const uint triangleCount = globalTCount;

    float d = INFINITY;
    int materialID = -1;
    uint lastT = LONGEST;
    vec3 normal;
    vec2 texcoord;
    vec3 tangent;
    for(uint i=0;i<min(vox.count, triangleCount);i++){
        if(!needsSkip){
            uint tri = hash.triangle;

            vec3 triverts[3];
            uint ji[3];
            for(uint x=0;x<3;x++){
                uint j = indics[tri * 3 + x]; ji[x] = j;
                triverts[x] = vec3(verts[j * 3 + 0], verts[j * 3 + 1], verts[j * 3 + 2]);
            }

            vec2 uv;
            float _d = intersect(orig, dir, triverts, uv);
            if(_d >= 0.0f && _d < d) {
                d = _d;
                lastT = tri;

                vec3 trinorms[3];
                vec2 texcoords[3];
                for(uint x=0;x<3;x++){
                    uint j = ji[x];
                    trinorms[x] = vec3(norms[j * 3 + 0], norms[j * 3 + 1], norms[j * 3 + 2]);
                    texcoords[x] = vec2(texcs[j * 2 + 0], texcs[j * 2 + 1]);
                }

                normal = cross(triverts[1] - triverts[0], triverts[2] - triverts[0]);
                normal = normalizeFast(trinorms[1]) * uv.x;
                normal += normalizeFast(trinorms[2]) * uv.y;
                normal += normalizeFast(trinorms[0]) * (1.0f - uv.x - uv.y);
                normal = normalizeFast(normal);

                texcoord = texcoords[1] * uv.x;
                texcoord += texcoords[2] * uv.y;
                texcoord += texcoords[0] * (1.0f - uv.x - uv.y);
                texcoord.y = 1.0f - texcoord.y;
                //texcoord = vec2(0.0);

                vec3 deltaPos;
                if(abs(dot(triverts[0] - triverts[1], vec3(1.0f))) < 0.0001f) {
                    deltaPos = triverts[2] - triverts[0];
                } else {
                    deltaPos = triverts[1] - triverts[0];
                }
                vec2 deltaUV1 = texcoords[1] - texcoords[0];
                vec2 deltaUV2 = texcoords[2] - texcoords[0];
                vec3 tan;
                if(deltaUV1.s != 0.0f) {
                    tan = deltaPos / deltaUV1.s;
                } else {
                    tan = deltaPos / 1.0f;
                }
                tan = normalizeFast(tan - dot(normal,tan)*normal);

                tangent = tan;
                materialID = mats[tri];
            }

            uint previous = hash.previous;
            if(previous != LONGEST){
                hash = thashes[previous];
            } else {
                needsSkip = true;
            }
        }
    }

    TResult res;
    res.dist = d;
    res.triangle = lastT;
    res.normal = normal;
    res.texcoord = texcoord;
    res.materialID = materialID;
    res.tangent = tangent;
    return res;
}

vec3 projectVoxels(in vec3 orig, in uint maxDepth){
    vec3 torig = orig;
    uint resX = uint(pow(2, maxDepth-1));
    torig -= offset;
    torig /= scale;
    torig *= resX;
    return torig;
}

float getScale(){
    return pow(2, maxDepth-1) / scale.x;
}

TResult traverse(vec3 orig, vec3 dir){
    uint currentDepth = maxDepth-1;
    uint resX = uint(pow(2, currentDepth));
    vec3 torig = projectVoxels(orig, currentDepth+1);
    vec3 ray_start = torig + dir * 0.0001f;
    vec2 d = intersectCube(ray_start, dir, vec3(0.0f) - 0.0001f, vec3(resX) + 0.0001f);
    float rate = getScale();

    TResult lastRes;
    lastRes.triangle = 0xFFFFFFFF;
    lastRes.dist = INFINITY;

    if(d.y >= 0.0f && d.x < INFINITY && d.y < INFINITY){
        if(d.x >= 0.0f) {
            ray_start = (torig + dir * (d.x + 0.0001f));
        }
        vec3 ray_end = (torig + dir * (d.y + 0.0001f));

        float _bin_size = 1.0f;
        vec3 p0 = ray_start / _bin_size;
        vec3 p1 = ray_end / _bin_size;

        vec3 step = sign(p1 - p0);
        if(abs(step.x) < 0.0001f) step.x = 1.0f;
        if(abs(step.y) < 0.0001f) step.y = 1.0f;
        if(abs(step.z) < 0.0001f) step.z = 1.0f;
        vec3 delta = _bin_size / abs(p1 - p0);
        vec3 test = floor(p0);
        vec3 mx = delta * mix(fract(p0), 1.0 - fract(p0), step * 0.5 + 0.5);

        const uint iterationCount = uint(ceil(float(resX) * sqrt(3.0f)));
        uint iteration = 0;

        do {
			iteration++;
		
            bool overlap = false;
            bool found = false;
            uvec3 coord = uvec3(test);
            Voxel vox = loadVoxel(coord, currentDepth);

            vec3 mnc = test;
            vec3 mxc = mnc + 1.0f;
            vec2 d = intersectCube(ray_start, dir, mnc, mxc);
            float rd = d.x / rate;

            if(
                d.y >= 0.0f && d.x < INFINITY && d.y < INFINITY && vox.count > 0
            ) {
                TResult lres = test_intersection(orig, dir, vox);
                if(lres.dist < lastRes.dist){
                    lastRes = lres;
                }
            } else {
                float coef = 1.0f;
                vec3 tst = test;
                vec3 coordf = tst;
                Voxel _vox = vox;
                float dX = 1.0f;
                for(int i=int(currentDepth)-1;i>=0;i--){
                    if(_vox.parent == LONGEST) break;
                    _vox = voxels[_vox.parent];

                    dX *= 2.0f;
                    coordf = floor(coordf / 2.0f);

                    vec3 mnc = coordf * dX - 0.0001f;
                    vec3 mxc = (coordf + 1.0f) * dX + 0.0001f;
                    vec2 d = intersectCube(ray_start, dir, mnc, mxc);
                    if(d.y >= 0.0f && d.x < INFINITY && d.y < INFINITY){
                        if(_vox.count > 0){
                            break;
                        } else {
                            found = true;
                            vec3 p0 = (ray_start + dir * d.y) / _bin_size;
                            test = floor(p0);
                            mx = delta * mix(fract(p0), 1.0 - fract(p0), step * 0.5 + 0.5);
                        }
                    }
                }
            }
			
			if(
                lastRes.triangle != LONGEST && 
				lastRes.dist < rd
            ) {
                return lastRes;
            }

            if(!found){
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
            }
        } while(
            iteration <= iterationCount &&

            test.x >= 0.0f &&
            test.y >= 0.0f &&
            test.z >= 0.0f &&

            test.x < float(resX) &&
            test.y < float(resX) &&
            test.z < float(resX)
        );
    }

    return lastRes;
}

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

uniform uint materialID;
uniform mat4 transform;
uniform mat4 transformInv;

void main()
{
    if(gl_GlobalInvocationID.x < rayCount){
        uint t = gl_GlobalInvocationID.x;

        Ray ray = rays[t];
        if(ray.actived > 0 && ray.params.y < ray.params.z){
            Hit hit = hits[ray.hit];

            vec3 p0 = ray.origin;
            vec3 p1 = ray.origin + ray.direct;
            vec3 p0t = (transformInv * vec4(ray.origin, 1.0)).xyz;
            vec3 p1t = (transformInv * vec4(ray.origin + ray.direct, 1.0)).xyz;

            Ray tray = ray;
            tray.origin = p0t;
            tray.direct = p1t - p0t;
            float rate = lengthFast(tray.direct) / lengthFast(ray.direct);
            tray.direct /= rate;

            TResult res = traverse(tray.origin, tray.direct);
            float dst = res.dist / rate;
            if(dst < hit.dist){
                Hit newHit = hit;
                newHit.dist = dst;
                newHit.normal = (vec4(res.normal, 0.0) * transformInv).xyz;
                newHit.triangle = res.triangle;
                if(res.materialID >= 0){
                    newHit.materialID = uint(res.materialID) + materialID;
                } else {
                    newHit.materialID = materialID;
                }
                newHit.texcoord = res.texcoord;
                newHit.tangent = (vec4(res.tangent, 0.0) * transformInv).xyz;
                storeHit(ray, newHit);
            }
        }
    }
}
