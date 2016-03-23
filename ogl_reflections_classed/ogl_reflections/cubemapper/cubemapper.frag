#version 450 core
#extension GL_NV_fragment_shader_interlock : enable

layout(sample_interlock_ordered) in;

const uint LONGEST = 0xFFFFFFFF;

out vec4 outColor;

flat in int face;
flat in vec3 vert0;
flat in vec3 vert1;
flat in vec3 vert2;
in int gl_PrimitiveID;
in vec3 position;

struct TexelInfo {
    vec3 position;
    //uvec3 texel;
    uint triangle;
    //float dist;
    uint previous;
};

layout(binding = 0) uniform atomic_uint texelInfoCounter;
layout(std430, binding=0) buffer s_texels {TexelInfo texelInfo[];};
layout(r32ui, binding = 0) uniform uimageCube texelCounts;
layout(r32ui, binding = 1) uniform uimageCube texelLasts;

uniform vec3 center;

void main(){
    vec2 coord = gl_FragCoord.xy;

    ivec3 cubeCoord0 = ivec3(floor(coord), face);
    beginInvocationInterlockNV();
    //for(uint x=0;x<1;x++){
        //for(uint y=0;y<1;y++){
            ivec3 cubeCoord = cubeCoord0;
            //cubeCoord.xy += ivec2(x, y);
            uint i = atomicCounterIncrement(texelInfoCounter);

            vec3 relative = position - center;
            float dist = length(relative);

            TexelInfo info;
            info.previous = imageAtomicExchange(texelLasts, cubeCoord, i);
            info.triangle = gl_PrimitiveID;
            info.position = position;
            //info.texel = cubeCoord;
            //info.dist = dist;
            texelInfo[i] = info;

            imageAtomicAdd(texelCounts, cubeCoord, 1);
        //}
    //}
    endInvocationInterlockNV();

    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}