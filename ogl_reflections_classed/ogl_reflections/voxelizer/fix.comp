#version 450 core
#extension GL_ARB_shading_language_include : require

#include </constants>
#include </uniforms>
#include </fastmath>
#include </octree>
#include </octree_helper>
layout (binding=0) uniform atomic_uint scounter;
layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

void main(){
    uint idx = gl_GlobalInvocationID.x;
    uint resX = uint(pow(2, currentDepth));

    if(currentDepth == 0) {
        if(idx == 0){
            uint hloc = atomicCounterIncrement(lscounter_to);
            to_helper[hloc] = 0;
            Voxel voxel;
            voxel.last = LONGEST;
            voxel.count = 0;
            voxel.coordX = 0;
            voxel.coordY = 0;
            voxel.coordZ = 0;
            voxel.parent = LONGEST;

            uint loc = atomicCounterIncrement(scounter);
            voxels[loc] = voxel;
        }
    } else {
        uint spr = idx / 8;
        if(spr < atomicCounter(lscounter_from)){
            uint parent = from_helper[spr];

            uint loc = idx % 8;
            uint w = loc % 2;
            uint h = (loc / 2) % 2;
            uint d = loc / 4;
            uvec3 co = uvec3(w, h, d);

            Voxel voxel = voxels[parent];
            voxels_subgrid[parent * 8 + loc] = 0xFFFFFFFF;
            if(voxel.count > 0){
                uvec3 parentSpace = uvec3(voxel.coordX, voxel.coordY, voxel.coordZ);
                uvec3 globalSpace = parentSpace * 2 + co;

                Voxel newVoxel;
                newVoxel.parent = parent;
                newVoxel.coordX = globalSpace.x;
                newVoxel.coordY = globalSpace.y;
                newVoxel.coordZ = globalSpace.z;
                newVoxel.count = 0;
                newVoxel.last = LONGEST;

                uint vloc = atomicCounterIncrement(scounter);
                uint hloc = atomicCounterIncrement(lscounter_to);
                to_helper[hloc] = vloc;

                voxels[vloc] = newVoxel;
                voxels_subgrid[parent * 8 + loc] = vloc;
            }
        }
    }
}