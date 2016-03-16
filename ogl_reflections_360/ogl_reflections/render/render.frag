#version 450 core
#extension GL_NV_gpu_program5 : enable

out vec4 outColor;

const uint LONGEST = 0xFFFFFFFF;
const float INFINITY = 1e10;

float len(vec3 a){
    return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

vec3 norm(vec3 a){
    return a / len(a);
}

float len(vec2 a){
    return sqrt(a.x * a.x + a.y * a.y);
}

vec2 norm(vec2 a){
    return a / len(a);
}

struct TexelInfo {
    vec3 position;
    //uvec3 texel;
    uint triangle;
    //float dist;
    uint previous;
};

float intersect(in vec3 orig, in vec3 dir, in vec3 v[3][4], inout int idx, inout vec2 uv){
    vec3 e11 = v[1][0] - v[0][0];
    vec3 e21 = v[2][0] - v[0][0];
    vec3 e12 = v[1][1] - v[0][1];
    vec3 e22 = v[2][1] - v[0][1];
    vec3 e13 = v[1][2] - v[0][2];
    vec3 e23 = v[2][2] - v[0][2];
    vec3 e14 = v[1][3] - v[0][3];
    vec3 e24 = v[2][3] - v[0][3];

    vec4 v0x = vec4(v[0][0].x, v[0][1].x, v[0][2].x, v[0][3].x);
    vec4 v0y = vec4(v[0][0].y, v[0][1].y, v[0][2].y, v[0][3].y);
    vec4 v0z = vec4(v[0][0].z, v[0][1].z, v[0][2].z, v[0][3].z);

    vec4 e1x = vec4(e11.x, e12.x, e13.x, e14.x);
    vec4 e1y = vec4(e11.y, e12.y, e13.y, e14.y);
    vec4 e1z = vec4(e11.z, e12.z, e13.z, e14.z);
    vec4 e2x = vec4(e21.x, e22.x, e23.x, e24.x);
    vec4 e2y = vec4(e21.y, e22.y, e23.y, e24.y);
    vec4 e2z = vec4(e21.z, e22.z, e23.z, e24.z);

    vec4 dir4x = dir.xxxx;
    vec4 dir4y = dir.yyyy;
    vec4 dir4z = dir.zzzz;

    vec4 pvecx = dir4y*e2z - dir4z*e2y;
    vec4 pvecy = dir4z*e2x - dir4x*e2z;
    vec4 pvecz = dir4x*e2y - dir4y*e2x;

    vec4 divisor = pvecx*e1x + pvecy*e1y + pvecz*e1z;
    vec4 invDivisor = vec4(1, 1, 1, 1) / divisor;

    vec4 orig4x = orig.xxxx;
    vec4 orig4y = orig.yyyy;
    vec4 orig4z = orig.zzzz;

    vec4 tvecx = orig4x - v0x;
    vec4 tvecy = orig4y - v0y;
    vec4 tvecz = orig4z - v0z;

    vec4 u4;
    u4 = tvecx*pvecx + tvecy*pvecy + tvecz*pvecz;
    u4 = u4 * invDivisor;

    vec4 qvecx = tvecy*e1z - tvecz*e1y;
    vec4 qvecy = tvecz*e1x - tvecx*e1z;
    vec4 qvecz = tvecx*e1y - tvecy*e1x;

    vec4 v4;
    v4 = dir4x*qvecx + dir4y*qvecy + dir4z*qvecz;
    v4 = v4 * invDivisor;

    vec4 t4 = vec4(INFINITY);
    t4 = e2x*qvecx + e2y*qvecy + e2z*qvecz;
    t4 = t4 * invDivisor;

    float t = INFINITY;

    if(t4.x < t && t4.x > 0)
    if(u4.x >= 0 && v4.x >= 0 && u4.x + v4.x <= 1) {
        t = t4.x;
        idx = 0;
        uv = vec2(u4.x, v4.x);
    }

    if(t4.y < t && t4.y > 0)
    if(u4.y >= 0 && v4.y >= 0 && u4.y + v4.y <= 1) {
         t = t4.y;
         idx = 1;
         uv = vec2(u4.y, v4.y);
    }

    if(t4.z < t && t4.z > 0)
    if(u4.z >= 0 && v4.z >= 0 && u4.z + v4.z <= 1) {
        t = t4.z;
        idx = 2;
        uv = vec2(u4.z, v4.z);
    }

    if(t4.w < t && t4.w > 0)
    if(u4.w >= 0 && v4.w >= 0 && u4.w + v4.w <= 1) {
        t = t4.w;
        idx = 3;
        uv = vec2(u4.w, v4.w);
    }

    return t;
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





vec3 generate_cubemap_coord(in vec2 txc, in int face)
{
    vec3 v;
    switch(face)
    {
        case 0: v = vec3( 1.0, -txc.y, -txc.x); break; // +X
        case 1: v = vec3(-1.0, -txc.y,  txc.x); break; // -X
        case 2: v = vec3( txc.x,  1.0,  txc.y); break; // +Y
        case 3: v = vec3( txc.x, -1.0, -txc.y); break; // -Y
        case 4: v = vec3( txc.x, -txc.y,  1.0); break; // +Z
        case 5: v = vec3(-txc.x, -txc.y, -1.0); break; // -Z
    }
    return norm(v);
}

vec3 get_coord(in vec3 v)
{
    const float d45 = 0.9999f;//0.70710678118f;
    float mx = max(abs(v.x), max(abs(v.y), abs(v.z)));
    vec3 vv = v / mx;
    vec3 vn = norm(v);
    vec3 f;
    if(dot(vv, vec3( 1.0,  0.0, 0.0)) > d45) f = vec3( -vv.z, -vv.y, 0.0f);
    if(dot(vv, vec3(-1.0,  0.0, 0.0)) > d45) f = vec3(  vv.z,  -vv.y, 1.0f);
    if(dot(vv, vec3( 0.0,  1.0, 0.0)) > d45) f = vec3(  vv.x,  vv.z, 2.0f);
    if(dot(vv, vec3( 0.0, -1.0, 0.0)) > d45) f = vec3(  vv.x,  -vv.z, 3.0f);
    if(dot(vv, vec3( 0.0,  0.0, 1.0)) > d45) f = vec3(   vv.x, -vv.y, 4.0f);
    if(dot(vv, vec3( 0.0,  0.0, -1.0)) > d45) f = vec3( -vv.x, -vv.y, 5.0f);
    return vec3(f.xy * 0.5 + 0.5, f.z);
}

layout(binding = 0) uniform atomic_uint texelInfoCounter;
layout(std430, binding=0) buffer s_texels {TexelInfo texelInfo[];};
layout(std430, binding=2) buffer s_vbo {float verts[];};
layout(std430, binding=3) buffer s_ebo {uint indics[];};
layout(std430, binding=4) buffer s_norm {float norms[];};
uniform usamplerCube texelCounts;
uniform usamplerCube texelLasts;
uniform vec3 center;
uniform mat4 projInv;
uniform mat4 camInv;
uniform vec2 sceneRes;

struct TResult {
    float dist;
    uint triangle;
    vec3 normal;
};


bool rinter(in vec2 x0, in vec2 n, in vec2 mn, in vec2 mx) {
    float tmin = -INFINITY, tmax = INFINITY;

    if (n.x != 0.0) {
        float tx1 = (mn.x - x0.x)/n.x;
        float tx2 = (mx.x - x0.x)/n.x;

        tmin = max(tmin, min(tx1, tx2));
        tmax = min(tmax, max(tx1, tx2));
    }

    if (n.y != 0.0) {
        float ty1 = (mn.y - x0.y)/n.y;
        float ty2 = (mx.y - x0.y)/n.y;

        tmin = max(tmin, min(ty1, ty2));
        tmax = min(tmax, max(ty1, ty2));
    }

    return tmax >= tmin;
}

#define LASTS 16

float distP(vec3 orig, vec3 dir, vec3 p0){
    return len(cross(dir, p0 - orig));
}

TResult traverse(in vec3 orig, in vec3 dir){
    vec3 origin = orig;

    float d = INFINITY;
    uint lastT = LONGEST;
    vec3 normal;

    float cubemapRes = textureSize(texelCounts, 0).x;

    origin += dir * 0.001f;

    float dst = 0.0f;
    bool needsSkip = false;
    uint triangleCount = uint(ceil(200000.0f / cubemapRes / sqrt(3.0f)));


    float ds = cubemapRes;

    uint sected[LASTS];
    for(uint x=0;x<LASTS;x++){
        sected[x] = LONGEST;
    }

    uint range = uint(ceil(cubemapRes * sqrt(3.0f)));
    for(uint i=0;i<range;i++){
        if(!needsSkip){
            vec3 samp = origin - center;
            float dist = len(samp);
            float step = 1.0f / ds;

            vec3 nsamp = norm(samp);
            vec3 coord01 = get_coord(nsamp);
            vec2 coord01_ = coord01.xy * cubemapRes;

            vec2 pdir;
            bool validpdir = false;
            {
                vec3 coord01t = get_coord(norm(origin + dir * 0.0001f - center));
                vec3 coord01tm = get_coord(norm(origin - dir * 0.0001f - center));
                coord01t.xy *= cubemapRes;
                coord01tm.xy *= cubemapRes;
                if(uint(coord01t.z) == uint(coord01.z)){
                    pdir = norm(vec2(coord01t.xy - coord01_.xy));
                    validpdir = true;
                } else
                if(uint(coord01tm.z) == uint(coord01.z)){
                    pdir = norm(vec2(coord01_.xy - coord01tm.xy));
                    validpdir = true;
                }
            }

            vec3 coord = vec3(coord01.xy * cubemapRes, coord01.z);
            float x0 = coord.x - 0.4999f;
            float y0 = coord.y - 0.4999f;
            for(uint w=0;w<4;w++){
                float x1 = floor(x0 + float(w % 2));
                float y1 = floor(y0 + floor(w / 2));
                uvec3 disp = uvec3(x1, y1, coord.z);

                vec2 mn = (disp.xy);
                vec2 mx = mn + 1.0f;
                vec3 sm = generate_cubemap_coord((mn + 0.5001f) / cubemapRes * 2.0 - 1.0, int(disp.z));

                uint count = texture(texelCounts, sm).r;
                uint last = texture(texelLasts, sm).r;
                uint dlast = last;

                if(last != LONGEST && count > 0 && rinter(coord01_, pdir, mn - 0.0001f, mx + 0.0001f)){
                    bool sect = false;
                    for(int y=LASTS-1;y>=0;y--){
                        if(!sect){
                            sect = (last == sected[y]);
                        } else {
                            break;
                        }
                    }
                    if(!sect){
                        uint cnt = min(count, triangleCount);
                        uint cnt4 = uint(ceil(float(cnt) / 4.0f));
                        for(uint j=0;j<cnt;j++){
                            if(last != LONGEST){

                                TexelInfo info = texelInfo[last];
                                vec3 p = info.position;
                                if(distP(orig, dir, p) < dist * 192.0f){

                                    uint c[3];
                                    vec3 triverts[3];
                                    uint tri = info.triangle;
                                    for(uint x=0;x<3;x++){
                                        uint ci = indics[tri * 3 + x];
                                        c[x] = ci;
                                        triverts[x] = vec3(verts[ci * 3 + 0], verts[ci * 3 + 1], verts[ci * 3 + 2]);
                                    }

                                    vec2 uv;
                                    float _d = intersect(orig, dir, triverts, uv);

                                    if(_d >= 0.0f && _d < d) {
                                        vec3 trinorms[3];
                                        for(uint x=0;x<3;x++){
                                            uint ci = c[x];
                                            trinorms[x] = vec3(norms[ci * 3 + 0], norms[ci * 3 + 1], norms[ci * 3 + 2]);
                                        }

                                        d = _d;
                                        lastT = tri;
                                        normal = norm(trinorms[1]) * uv.x;
                                        normal += norm(trinorms[2]) * uv.y;
                                        normal += norm(trinorms[0]) * (1.0f - uv.x - uv.y);
                                    }
                                }

                                last = info.previous;
                            }
                        }
                        for(int i=0;i<LASTS-1;i++){
                            sected[i] = sected[i+1];
                        }
                        sected[LASTS-1] = dlast;
                    }
                }
            }

            if(lastT != LONGEST && d >= 0.0f && d < INFINITY) {
                TResult res;
                res.dist = d;
                res.triangle = lastT;
                res.normal = normal;
                needsSkip = true;
                return res;
                break;
            }

            origin += dir * step * dist;
            dst += step * dist;
            if(dst >= 100.0f) {
                needsSkip = true;
            }
        } else {
            break;
        }
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
    vec3 dir = norm(co.xyz - orig.xyz);
    vec3 color = vec3(0.0f);
    float rating = 1.0f;

    float cubemapRes = textureSize(texelCounts, 0).x;
    uint last = texture(texelLasts, dir).r;
    uint count = texture(texelCounts, dir).r;

    float d = INFINITY;
    uint lastT = LONGEST;
    vec3 normal;

    TResult res;

    for(int i=0;i<2;i++){
        res = traverse(orig, dir);
        if(res.triangle != LONGEST){
            orig += dir * res.dist;

            if(i == 0)
                rating = 1.0f;
            else
                rating = 0.25f;

            vec3 normal = norm(res.normal * -dot(dir, res.normal));
            float light = dot(norm(light - orig), normal) * 0.5 + 0.5;

            color = mix(color, vec3(0.9f, 0.5f, 0.5f) * light, rating);

            dir = reflect(dir, normal);
            orig += norm(normal * dot(dir, normal)) * 0.0001f;
        }
    }


    outColor = vec4(color, 1.0f);
}