#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#extension GL_NV_gpu_shader5 : enable

#include </constants>
#include </uniforms>
#include </fastmath>
#include </octree>
#include </octree_raw>
layout (binding=1) uniform atomic_uint vcounter;

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

uvec2 searchVoxelIndexAlt(uvec3 idx){
    uvec3 id = uvec3(0);
    bool fail = false;
    uint sub = 0;
    Voxel hash = voxels[sub];
    uvec3 idu = uvec3(0);
    uint grid = 0;

    int end = int(maxDepth)-1;
    int strt = end - int(currentDepth);
    for(int i=end;i>=strt;i--){
        if(!fail){
            id = uvec3(idx / uint(pow(2, i)));
            idu = id % uvec3(2);
            if(i == end){
                grid = 0;
                sub = 0;
            } else {
                grid = cnv_subgrid(sub, idu);
                sub = voxels_subgrid[grid];
            }
            if(i > 0 && sub == LONGEST) {
                fail = true;
            } else {
                hash = voxels[sub];
            }
        } else {
            break;
        }
    }

    if(!fail){
        return uvec2(sub, grid);
    }
    return uvec2(LONGEST);
}

void main(){
    uint t = gl_GlobalInvocationID.x;
    if(t < atomicCounter(dcounter)){
        VoxelRaw tri = voxels_raw[t];
        uvec3 coord = uvec3(tri.coordX, tri.coordY, tri.coordZ);
        uvec2 t = searchVoxelIndexAlt(coord);
        atomicAdd(voxels[t.x].count, 1);
        if(currentDepth == maxDepth-1){
            uint i = atomicCounterIncrement(vcounter);
            Thash thash;
            thash.triangle = tri.triangle;
            thash.previous = atomicExchange(voxels[t.x].last, i);
            thashes[i] = thash;
        }
     }
}