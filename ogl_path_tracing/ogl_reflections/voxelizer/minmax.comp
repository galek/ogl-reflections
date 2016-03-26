#version 450 core

struct Minmax {
    vec3 mn;
    vec3 mx;
};

uniform uint tcount;
uniform uint from;
uniform uint prev;
uniform uint count;

layout(std430, binding=0) buffer s_minmax {Minmax minmax[];};
layout(std430, binding=1) buffer s_vbo {float verts[];};
layout(std430, binding=2) buffer s_ebo {uint indics[];};

layout (binding=0) uniform atomic_uint mcounter;

const uint LONGEST = 0xFFFFFFFF;
layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

void main(){
    uint idx = gl_GlobalInvocationID.x;
    if(idx < count){
        Minmax mnmx;
        mnmx.mn = vec3(10000.0f);
        mnmx.mx = vec3(-10000.0f);

        if(from < 1){
            for(uint i=0;i<2;i++){
                vec3 triverts[3];
                uint tri = idx * 2 + i;
                if(tri < tcount){
                    for(uint x=0;x<3;x++){
                        uint j = indics[tri * 3 + x];
                        triverts[x] = vec3(verts[j * 3 + 0], verts[j * 3 + 1], verts[j * 3 + 2]);
                    }
                    mnmx.mn = min(mnmx.mn, min(triverts[0], min(triverts[1], triverts[2])));
                    mnmx.mx = max(mnmx.mx, max(triverts[0], max(triverts[1], triverts[2])));
                }
            }
        } else {
            for(uint i=0;i<2;i++){
                uint id = prev + idx * 2 + i;
                if(id < from) {
                    Minmax pv = minmax[id];
                    mnmx.mn = min(mnmx.mn, pv.mn);
                    mnmx.mx = max(mnmx.mx, pv.mx);
                }
            }
        }

        minmax[from + idx] = mnmx;
        atomicCounterIncrement(mcounter);
    }
}