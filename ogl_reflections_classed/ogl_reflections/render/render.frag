#version 450 core

out vec4 outColor;

const uint LONGEST = 0xFFFFFFFF;
const float INFINITY = 1e10;

/* ============ RAYS ============= */

struct Ray {
    uint previous;
    uint hit;
    int actived;
    vec3 origin;
    vec3 direct;
    vec3 color;
    vec3 final;
    vec4 params;
    vec4 params0;
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

struct Texel {
    uint last;
    uint count;
};

layout(std430, binding=8) buffer s_rays {Ray rays[];};
layout(std430, binding=9) buffer s_hits {Hit hits[];};
layout(std430, binding=10) buffer s_texels {Texel texelInfo[];};
layout (binding=0) uniform atomic_uint rcounter;
layout (binding=1) uniform atomic_uint hcounter;

uint createRay(in Ray original){
    uint hidx = atomicCounterIncrement(hcounter);
    uint ridx = atomicCounterIncrement(rcounter);
    Ray ray = original;
    ray.hit = hidx;
    Hit hit;
    hit.dist = 10000.0f;
    hit.triangle = LONGEST;
    hit.materialID = LONGEST;
    hits[hidx] = hit;
    rays[ridx] = ray;
    return ridx;
}

uint createRay(){
    Ray newRay;
    return createRay(newRay);
}

void storeTexel(in uint texel, in uint rayIndex){
    rays[rayIndex].previous = atomicExchange(texelInfo[texel].last, rayIndex);
    atomicAdd(texelInfo[texel].count, 1);
}

void storeRay(in uint rayIndex, in Ray ray){
    rays[rayIndex].direct = ray.direct;
    rays[rayIndex].origin = ray.origin;
    rays[rayIndex].actived = ray.actived;
    rays[rayIndex].color = ray.color;
    rays[rayIndex].final = ray.final;
    rays[rayIndex].params = ray.params;
    rays[rayIndex].params0 = ray.params0;
}

void storeHit(in Ray ray, in Hit hit){
    hits[ray.hit].dist = hit.dist;
    hits[ray.hit].triangle = hit.triangle;
    hits[ray.hit].normal = hit.normal;
    hits[ray.hit].texcoord = hit.texcoord;
    hits[ray.hit].tangent = hit.tangent;
    hits[ray.hit].materialID = hit.materialID;
    hits[ray.hit].params = hit.params;
}

Ray fetchFirstRay(in uint texel){
    return rays[texelInfo[texel].last];
}

Ray fetchNextRay(in Ray ray){
    return rays[ray.previous];
}

/* ============ END =========== */

layout(std430, binding=11) buffer s_samp {vec4 samples[];};

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