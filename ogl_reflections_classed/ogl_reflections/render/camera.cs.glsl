#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#extension GL_NV_gpu_shader5 : enable
#include </structs>
#include </constants>
#include </uniforms>
#include </random>
#include </fastmath>

uniform mat4 projInv;
uniform mat4 camInv;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

void main()
{
    if(gl_GlobalInvocationID.x < sceneRes.x && gl_GlobalInvocationID.y < sceneRes.y){
        uint t = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * uint(sceneRes.x);
        texelInfo[t].count = 0;
        texelInfo[t].last = LONGEST;

        for(int i=0;i<1;i++){
            vec2 coord = vec2(gl_GlobalInvocationID.xy);
            coord += vec2(random(), random());
            coord /= sceneRes;
            coord = coord * 2.0f - 1.0f;

            vec4 co = camInv * projInv * vec4(coord, 0.0f, 1.0f);
            co /= co.w;
            vec4 ce = camInv * vec4(0.0f, 0.0f, 0.0f, 1.0f);

            vec3 orig = ce.xyz;
            vec3 dir = normalizeFast(co.xyz - orig.xyz);

            Ray ray;
            ray.direct = dir;
            ray.origin = orig;
            ray.actived = 1;
            ray.color = vec3(1.0f);
            ray.final = vec3(0.0f);
            ray.params.x = 1.0f;
            ray.params.y = 0.0f;
            ray.params.z = 4.0f;
            ray.params.w = 0.0f;
            ray.shadow = LONGEST;
            ray.params0.x = 1.0f;
            ray.params0.y = 1.0f;
            storeTexel(t, createRay(ray));
        }
    }
}