#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#extension GL_NV_gpu_shader5 : enable
#include </constants>
#include </uniforms>
#include </structs>

out vec4 outColor;

void main()
{
    uvec2 globalInvocationID = uvec2(floor(gl_FragCoord.xy));
    if(globalInvocationID.x < sceneRes.x && globalInvocationID.y < sceneRes.y){
        uint t = globalInvocationID.x + globalInvocationID.y * uint(sceneRes.x);
        Ray ray = fetchFirstRay(t);
        uint count = texelInfo[t].count;
        outColor = vec4(samples[t].xyz / samples[t].w, 1.0f);
    }
}