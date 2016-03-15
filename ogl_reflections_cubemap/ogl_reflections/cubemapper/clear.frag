#version 450 core

const uint LONGEST = 0xFFFFFFFF;

out vec4 outColor;

flat in int face;
flat in vec3 vert0;
flat in vec3 vert1;
flat in vec3 vert2;
in int gl_PrimitiveID;

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
    ivec3 cubeCoord = ivec3(floor(coord), face);
    imageStore(texelLasts, cubeCoord, uvec4(0xFFFFFFFF));
    imageStore(texelCounts, cubeCoord, uvec4(0));
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}