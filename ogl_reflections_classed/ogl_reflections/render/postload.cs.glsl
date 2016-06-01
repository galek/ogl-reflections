#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#extension GL_NV_gpu_shader5 : enable
#include </structs>
#include </constants>
#include </uniforms>
#include </fastmath>
#include </fimages>

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

void main()
{
        uint t = gl_GlobalInvocationID.x;
        if(t < rayCount){
            Ray ray = rays[t];
            Hit hit = hits[ray.hit];
            ivec2 uv = ivec2(t % 1024, t / 1024);
            float depth = imageLoad(images0, uv).x;
            if(depth <= hit.dist){
                hit.dist = depth;
                hit.materialID = uint(int(imageLoad(images0, uv).y));
                hit.triangle = uint(int(imageLoad(images0, uv).z));
                hit.normal = imageLoad(images1, uv).xyz;
                hit.tangent = imageLoad(images2, uv).xyz;
                hit.texcoord = imageLoad(images3, uv).xy;
                storeHit(ray, hit);
            }
        }
}
