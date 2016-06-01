#version 450 compatibility
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_shading_language_include : require
#extension GL_NV_fragment_shader_interlock : enable

layout (sample_interlock_unordered) in;

#include </constants>
#include </uniforms>
#include </fastmath>
#include </structs>
#include </vertex_attributes>

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

uniform uint materialID;
uniform mat4 transform;
uniform mat4 transformInv;

flat in int materialIDp;
flat in vec3 vert0;
flat in vec3 vert1;
flat in vec3 vert2;
flat in vec3 normal0;
flat in vec3 normal1;
flat in vec3 normal2;
flat in vec2 texcoord0;
flat in vec2 texcoord1;
flat in vec2 texcoord2;
flat in int gl_PrimitiveID;

uniform vec2 resolution;

void main()
{
    uint t = uint(floor(gl_FragCoord.x) + resolution.x * floor(gl_FragCoord.y));
    vec2 uv = vec2(0.0f);
    float dst = 10000.0f;

    gl_FragData[0] = vec4(dst, -1.0f, -1.0f, 1.0f);
    gl_FragData[1] = vec4(vec3(0.0f), 1.0f);
    gl_FragData[2] = vec4(vec3(0.0f), 1.0f);
    gl_FragData[3] = vec4(vec3(0.0f), 1.0f);
    gl_FragDepth = clamp(dst / 10000.0f, 0.0f, 1.0f);

    if(t < rayCount){
        Ray ray = rays[t];

        vec3 p0 = ray.origin;
        vec3 p1 = ray.origin + ray.direct;
        vec3 p0t = (transformInv * vec4(ray.origin, 1.0)).xyz;
        vec3 p1t = (transformInv * vec4(ray.origin + ray.direct, 1.0)).xyz;

        Ray tray = ray;
        tray.origin = p0t;
        tray.direct = p1t - p0t;
        float rate = lengthFast(tray.direct) / lengthFast(ray.direct);
        tray.direct /= rate;

        //Calculate vertices
        vec3 verts[3];
        verts[0] = vert0;
        verts[1] = vert1;
        verts[2] = vert2;

        //Calculate distance
        float dist = 10000.0f;
        if(ray.actived > 0 && ray.params.y < ray.params.z){
            dist = intersect(tray.origin, tray.direct, verts, uv);
            dst = dist / rate;
            if(dst > 0.0001f && dst < 10000.0f){
                //Calculate normal
                vec3 norm = normal1 * uv.x + normal2 * uv.y + normal0 * (1.0f - uv.x - uv.y);
                vec3 normal = (vec4(norm, 0.0) * transformInv).xyz;

                //Calculate indexes
                uint triangle = gl_PrimitiveID;
                uint materialID = uint(materialID);
                if(materialID >= 0){
                    materialID = uint(materialID) + materialIDp;
                } else {
                    materialID = materialIDp;
                }

                //Calculate tangent
                vec3 deltaPos;
                if(abs(dot(vert0 - vert1, vec3(1.0f))) < 0.0001f) {
                    deltaPos = vert2 - vert0;
                } else {
                    deltaPos = vert1 - vert0;
                }
                vec2 deltaUV1 = texcoord1 - texcoord0;
                vec2 deltaUV2 = texcoord2 - texcoord0;
                vec3 tan;
                if(deltaUV1.s != 0.0f) {
                    tan = deltaPos / deltaUV1.s;
                } else {
                    tan = deltaPos / 1.0f;
                }
                tan = normalizeFast(tan - dot(norm,tan)*norm);

                //Define results
                vec2 texcoord = texcoord1 * uv.x + texcoord2 * uv.y + texcoord0 * (1.0f - uv.x - uv.y);
                vec3 tangent = (vec4(tan, 0.0) * transformInv).xyz;

                //All data
                gl_FragDepth = clamp(dst / 10000.0f, 0.0f, 1.0f);
                gl_FragData[0] = vec4(dst, float(int(materialID)), float(int(triangle)), 1.0f);
                gl_FragData[1] = vec4(normal, 1.0f);
                gl_FragData[2] = vec4(tangent, 1.0f);
                gl_FragData[3] = vec4(texcoord, 0.0f, 1.0f);
            }
        }
    }
}
