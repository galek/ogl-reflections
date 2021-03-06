#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#extension GL_NV_gpu_shader5 : enable
#include </structs>
#include </constants>
#include </uniforms>
layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

uniform samplerCube cubeTex;

void main()
{
    if(gl_GlobalInvocationID.x < rayCount){
        uint t = gl_GlobalInvocationID.x;

        Ray ray = rays[t];
        Hit hit = hits[ray.hit];

        if(
            ray.actived > 0 &&
            ray.params0.x > 0.0f &&
            ray.params.y < ray.params.z &&
            hit.dist >= 10000.0f
        ) {
            ray.final = ray.color * texture(cubeTex, ray.direct).xyz;
            ray.actived = 0;
        }

        if(
            hit.dist >= 10000.0f ||
            ray.params.w < 1.0f ||
            ray.params.y >= ray.params.z ||
            dot(ray.color, vec3(1.0f)) < 0.01f || ray.params0.y < 0.01f
        ){
            ray.actived = 0;
        }

        ray.params.w = 0.0f;
        storeRay(t, ray);
    }
}
