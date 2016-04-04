#include </constants>

struct Ray {
    uint texel;
    uint previous;
    uint hit;
    uint shadow;
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
layout(std430, binding=11) buffer s_samp {vec4 samples[];};
layout (binding=0) uniform atomic_uint rcounter;
layout (binding=1) uniform atomic_uint hcounter;

uint createRay(in Ray original){
    uint hidx = atomicCounterIncrement(hcounter);
    uint ridx = atomicCounterIncrement(rcounter);
    Ray ray = original;
    ray.previous = LONGEST;
    ray.hit = hidx;
    ray.shadow = LONGEST;
    Hit hit;
    hit.dist = 100000.0f;
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
    rays[rayIndex].texel = texel;
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
    rays[rayIndex].shadow = ray.shadow;
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