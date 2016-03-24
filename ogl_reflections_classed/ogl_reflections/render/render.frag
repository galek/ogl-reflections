#version 450 core

out vec4 outColor;

const uint LONGEST = 0xFFFFFFFF;
const float INFINITY = 1e10;

layout(std430, binding=11) buffer s_samp {vec4 samples[];};

uniform vec2 sceneRes;

void main()
{
    uvec2 globalInvocationID = uvec2(floor(gl_FragCoord.xy));
    if(globalInvocationID.x < sceneRes.x && globalInvocationID.y < sceneRes.y){
        uint t = globalInvocationID.x + globalInvocationID.y * uint(sceneRes.x);
        outColor = vec4(samples[t].xyz / samples[t].w, 1.0f);
    }
}