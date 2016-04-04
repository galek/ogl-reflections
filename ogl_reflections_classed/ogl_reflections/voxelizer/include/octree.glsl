struct Thash {
	uint triangle;
	uint previous;
};

struct Voxel {
    uint parent;
	uint last;
    uint count;
    uint coordX;
	uint coordY;
	uint coordZ;
};

layout(std430, binding=0) buffer s_voxels {Voxel voxels[];};
layout(std430, binding=1) buffer s_voxels_sub {uint voxels_subgrid[];};
layout(std430, binding=2) buffer s_thashes {Thash thashes[];};

uint cnv_subgrid(uint sub, uvec3 idu){
    return sub * 8 + (idu.x + idu.y * 2 + idu.z * 4);
}

uint searchVoxelIndex(uvec3 idx, uint maxDepth){
    uint currentDepth = maxDepth-1;
    uvec3 id = uvec3(0);
    bool fail = false;
    uint sub = 0;
    Voxel hash = voxels[sub];
    uvec3 idu = uvec3(0);
    uint grid = 0;

    int strt = int(currentDepth);
    for(int i=strt;i>=0;i--){
        if(!fail){
            id = uvec3(idx / uint(pow(2, i)));
            idu = id % uvec3(2);
            if(i == strt){
                grid = 0;
                sub = 0;
            } else {
                grid = cnv_subgrid(sub, idu);
                sub = voxels_subgrid[grid];
            }
            if(sub == LONGEST) {
                fail = true;
            } else {
                hash = voxels[sub];
            }
        } else {
            break;
        }
    }

    if(!fail){
        return sub;
    }
    return LONGEST;
}

uint searchVoxelIndex(uvec3 idx){
    return searchVoxelIndex(idx, maxDepth);
}

Voxel loadVoxel(in uvec3 coord, in uint depth){
    if(coord.x != LONGEST){
        uint idx = searchVoxelIndex(coord, depth+1);
        return voxels[idx];
    } else {
        Voxel voxel;
        voxel.last = LONGEST;
        voxel.count = 0;
        return voxel;
    }
}

Voxel loadVoxel(in uvec3 coord){
    return loadVoxel(coord, maxDepth-1);
}
