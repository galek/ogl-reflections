#version 450 core

out vec4 outColor;

const uint LONGEST = 0xFFFFFFFF;
const float INFINITY = 1e10;

struct Ray {
    int actived;
    vec3 origin;
    vec3 direct;
    vec3 color;
    vec3 final;
    vec4 params;
};

struct Hit {
    float dist;
    uint triangle;
    uint materialID;
    vec3 normal;
    vec3 tangent;
    vec2 texcoord;
    vec4 params;
};

layout(std430, binding=8) buffer s_rays {Ray rays[];};
layout(std430, binding=9) buffer s_hits {Hit hits[];};
layout(std430, binding=10) buffer s_samp {vec4 samples[];};

uniform vec2 sceneRes;

void main()
{
    uvec2 globalInvocationID = uvec2(floor(gl_FragCoord.xy));
    if(globalInvocationID.x < sceneRes.x && globalInvocationID.y < sceneRes.y){
        uint t = globalInvocationID.x + globalInvocationID.y * uint(sceneRes.x);
        Ray ray = rays[t];
        Hit hit = hits[t];
        outColor = vec4(samples[t].xyz / samples[t].w, 1.0f);
    }
}