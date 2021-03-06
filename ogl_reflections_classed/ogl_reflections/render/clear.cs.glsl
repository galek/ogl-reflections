#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#extension GL_NV_gpu_shader5 : enable
#include </uniforms>
#include </structs>

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

void main()
{
    if(gl_GlobalInvocationID.x < sceneRes.x && gl_GlobalInvocationID.y < sceneRes.y){
        uint t = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * uint(sceneRes.x);
        samples[t] = vec4(0.0f);
    }
}