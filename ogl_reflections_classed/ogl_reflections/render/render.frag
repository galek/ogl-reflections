#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#include </constants>
#include </uniforms>

out vec4 outColor;

layout(std430, binding=11) buffer s_samp {vec4 samples[];};

void main()
{
    uvec2 globalInvocationID = uvec2(floor(gl_FragCoord.xy));
    if(globalInvocationID.x < sceneRes.x && globalInvocationID.y < sceneRes.y){
        uint t = globalInvocationID.x + globalInvocationID.y * uint(sceneRes.x);
        outColor = vec4(samples[t].xyz / samples[t].w, 1.0f);
    }
}