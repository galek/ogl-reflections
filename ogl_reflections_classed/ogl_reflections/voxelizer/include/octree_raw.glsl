struct VoxelRaw {
    uint coordX;
    uint coordY;
    uint coordZ;
    uint triangle;
};

layout(std430, binding=8) coherent buffer s_voxels_raw {VoxelRaw voxels_raw[];};
layout (binding=0) uniform atomic_uint dcounter;
