#version 450 core

layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 18) out;

in vec3 verts[];
in int gl_PrimitiveIDIn;

flat out int face;
out vec3 position;
flat out vec3 vert0;
flat out vec3 vert1;
flat out vec3 vert2;
out int gl_PrimitiveID;

uniform mat4 proj;
uniform mat4 cams[6];
uniform vec3 center;

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

uniform vec2 cubeRes;

void main(){
    gl_PrimitiveID = gl_PrimitiveIDIn;

    for(int j = 0; j < 6; ++j) {
        face = j;
        for(int i = 0; i < 3; ++i) {
            position = verts[i];
            //gl_Position = proj * cams[j] * vec4(verts[i] - center, 1.0f);
            vec3 ver = verts[i];
            gl_Position = vec4(ver, 1.0f);
            EmitVertex();
        }
        EndPrimitive();
    }
}