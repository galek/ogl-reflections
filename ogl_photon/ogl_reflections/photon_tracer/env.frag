#version 450 core

out vec4 outColor;

const uint LONGEST = 0xFFFFFFFF;
const float INFINITY = 1e10;

flat in int face;

/* ============ RAYS ============= */

struct Ray {
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
layout (binding=0) uniform atomic_uint rcounter;
layout (binding=1) uniform atomic_uint hcounter;
layout (binding=3) uniform atomic_uint pcounter;

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

uniform uint photonCount;
uniform mat4 proj;
uniform mat4 cams[6];
uniform vec3 center;
uniform vec2 cubeRes;
uniform samplerCube cubeTex;

vec3 generate_cubemap_coord(in vec2 txc, in int face)
{
    vec3 v;
    switch(face)
    {
        case 0: v = vec3( 1.0, -txc.y, -txc.x); break; // +X
        case 1: v = vec3(-1.0, -txc.y,  txc.x); break; // -X
        case 2: v = vec3( txc.x,  1.0,  txc.y); break; // +Y
        case 3: v = vec3( txc.x, -1.0, -txc.y); break; // -Y
        case 4: v = vec3( txc.x, -txc.y,  1.0); break; // +Z
        case 5: v = vec3(-txc.x, -txc.y, -1.0); break; // -Z
    }
    return normalize(v);
}


uniform float time;

uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }

uint counter = 0;
float random(){
    float r = random(vec3(/*gl_GlobalInvocationID.xy*/ vec2(0.0f), time + counter));
    counter++;
    return r;
}


void main()
{
    uvec2 globalInvocationID = uvec2(floor(gl_FragCoord.xy));
    if(globalInvocationID.x < cubeRes.x && globalInvocationID.y < cubeRes.y){
        uint t = atomicCounterIncrement(pcounter);
        vec3 orig = center;
        vec3 dir = generate_cubemap_coord((vec2(globalInvocationID.xy) + vec2(random(), random()))/cubeRes*2.0-1.0,face);
        orig += dir * 9000.0f;
        vec3 color = texture(cubeTex, dir).xyz;

        Ray ray;
        ray.direct = -dir;
        ray.origin = orig;
        ray.actived = 1;
        ray.color = color;//vec3(2.0f);
        ray.final = vec3(0.0f);
        ray.params.x = 1.0f;
        ray.params.y = 0.0f;
        ray.params.z = 3.0f;
        ray.params.w = 0.0f;
        ray.shadow = LONGEST;
        ray.params0.x = 1.0f;
        storeTexel(t, createRay(ray));
    }
}