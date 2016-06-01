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
        ivec2 uv = ivec2(t % 1024, t / 1024);
        if(t < rayCount){
            Ray ray = rays[t];
            Hit hit = hits[ray.hit];
            ivec2 uv = ivec2(t % 1024, t / 1024);
            imageStore(images0, uv, vec4(10000.0f, -1.0f, -1.0f, 1.0f));
            imageStore(images1, uv, vec4(vec3(0.0f), 1.0f));
            imageStore(images2, uv, vec4(vec3(0.0f), 1.0f));
            imageStore(images3, uv, vec4(vec3(0.0f), 1.0f));
        }
}
