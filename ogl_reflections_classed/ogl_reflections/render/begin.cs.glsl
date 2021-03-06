#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#extension GL_NV_gpu_shader5 : enable
#include </structs>
#include </constants>
#include </uniforms>

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

void main()
{
    if(gl_GlobalInvocationID.x < rayCount){
        uint t = gl_GlobalInvocationID.x;
        Ray ray = rays[t];
        Hit hit;
        hit.dist = 10000.0f;
        hit.triangle = LONGEST;
        hit.materialID = LONGEST;
        hit.normal = vec3(0.0f);
        hit.tangent = vec3(0.0f);
        storeHit(ray, hit);
    }
}
