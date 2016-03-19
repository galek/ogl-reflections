#version 450 core
#extension GL_NV_fragment_shader_interlock : enable

layout(sample_interlock_unordered) in;

const uint LONGEST = 0xFFFFFFFF;

out vec4 outColor;

flat in int shortest;
in vec3 fragPosition;
flat in vec3 vert0;
flat in vec3 vert1;
flat in vec3 vert2;
in int gl_PrimitiveID;

struct Thash {
	uint triangle;
	uint previous;
};

struct Voxel {
	uint last;
    uint count;
    uint coordX;
    uint coordY;
    uint coordZ;
};

layout(std430, binding=0) coherent buffer s_voxels {Voxel voxels[];};
layout(std430, binding=1) coherent buffer s_voxels_sub {uint voxels_subgrid[];};
layout(std430, binding=2) coherent buffer s_thashes {Thash thashes[];};
layout (binding=0) uniform atomic_uint vcounter;
layout (binding=1) uniform atomic_uint scounter;

uniform uint currentDepth;



uint cnv_subgrid(uint sub, uvec3 idu){
    return sub * 8 + idu.x + idu.y * 2 + idu.z * 4;
}

uvec2 searchVoxelIndex(uvec3 idx){
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

#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if(x1<min) min=x1;\
  if(x1>max) max=x1;\
  if(x2<min) min=x2;\
  if(x2>max) max=x2;

int planeBoxOverlap(in vec3 normal, in vec3 vert, in vec3 maxbox)
{
    int q;
    vec3 vmin,vmax;

    vmin = mix( maxbox - vert, -maxbox - vert, sign(normal) * 0.5f + 0.5f);
    vmax = mix(-maxbox - vert,  maxbox - vert, sign(normal) * 0.5f + 0.5f);

    if(dot(normal,vmin)>0.0f) return 0;
    if(dot(normal,vmax)>=0.0f) return 1;
    return 0;
}

#define AXISTEST_X01(a, b, fa, fb)			   \
	p0 = a*v0.y - b*v0.z;			       	   \
	p2 = a*v2.y - b*v2.z;			       	   \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
	rad = fa * boxhalfsize.y + fb * boxhalfsize.z;   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_X2(a, b, fa, fb)			   \
	p0 = a*v0.y - b*v0.z;			           \
	p1 = a*v1.y - b*v1.z;			       	   \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize.y + fb * boxhalfsize.z;   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Y02(a, b, fa, fb)			   \
	p0 = -a*v0.x + b*v0.z;		      	   \
	p2 = -a*v2.x + b*v2.z;	       	       	   \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
	rad = fa * boxhalfsize.x + fb * boxhalfsize.z;   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Y1(a, b, fa, fb)			   \
	p0 = -a*v0.x + b*v0.z;		      	   \
	p1 = -a*v1.x + b*v1.z;	     	       	   \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize.x + fb * boxhalfsize.z;   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Z12(a, b, fa, fb)			   \
	p1 = a*v1.x - b*v1.y;			           \
	p2 = a*v2.x - b*v2.y;			       	   \
        if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
	rad = fa * boxhalfsize.x + fb * boxhalfsize.y;   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Z0(a, b, fa, fb)			   \
	p0 = a*v0.x - b*v0.y;				   \
	p1 = a*v1.x - b*v1.y;			           \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize.x + fb * boxhalfsize.y;   \
	if(min>rad || max<-rad) return 0;

int triBoxOverlap(in vec3 boxcenter, in vec3 boxhalfsize, in vec3 triverts[3]){
   vec3 v0,v1,v2;
   float min,max,p0,p1,p2,rad,fex,fey,fez;
   vec3 normal,e0,e1,e2;

   v0 = triverts[0] - boxcenter;
   v1 = triverts[1] - boxcenter;
   v2 = triverts[2] - boxcenter;

   e0= v1-v0;
   e1= v2-v1;
   e2= v0-v2;

   fex = abs(e0.x);
   fey = abs(e0.y);
   fez = abs(e0.z);

   AXISTEST_X01(e0.z, e0.y, fez, fey);
   AXISTEST_Y02(e0.z, e0.x, fez, fex);
   AXISTEST_Z12(e0.y, e0.x, fey, fex);

   fex = abs(e1.x);
   fey = abs(e1.y);
   fez = abs(e1.z);

   AXISTEST_X01(e1.z, e1.y, fez, fey);
   AXISTEST_Y02(e1.z, e1.x, fez, fex);
   AXISTEST_Z0(e1.y, e1.x, fey, fex);

   fex = abs(e2.x);
   fey = abs(e2.y);
   fez = abs(e2.z);

   AXISTEST_X2(e2.z, e2.y, fez, fey);
   AXISTEST_Y1(e2.z, e2.x, fez, fex);
   AXISTEST_Z12(e2.y, e2.x, fey, fex);

   FINDMINMAX(v0.x,v1.x,v2.x,min,max);
   if(min>boxhalfsize.x || max<-boxhalfsize.x) return 0;

   FINDMINMAX(v0.y,v1.y,v2.y,min,max);
   if(min>boxhalfsize.y || max<-boxhalfsize.y) return 0;

   FINDMINMAX(v0.z,v1.z,v2.z,min,max);
   if(min>boxhalfsize.z || max<-boxhalfsize.z) return 0;

   normal = cross(e0,e1);
   if(planeBoxOverlap(normal,v0,boxhalfsize) < 1) return 0;
   return 1;
}




#define BIAS 4

void main(){
    vec3 coord = gl_FragCoord.xyz;
    uint resX = uint(pow(2, currentDepth));
    coord.z *= float(resX);
    coord = floor(coord - 0.5f + 0.0001f) + 0.5f;

    float ce = floor(float(BIAS - 1) / 2.0);
    float z[BIAS];
    for(int x=0;x<BIAS;x++){
        z[x] = coord.z + float(x) - ce;
    }

    vec3 triverts[3];
    triverts[0] = vert0;
    triverts[1] = vert1;
    triverts[2] = vert2;

    bool overlap[BIAS];
    vec3 norms[BIAS];
    for(int x=0;x<BIAS;x++){
        vec3 norm = vec3(coord.xy, z[x]);
        if(shortest == 1) {
            norm.xyz = norm.xzy;
        } else
        if(shortest == 0) {
            norm.xyz = norm.zyx;
        }

        overlap[x] =
            norm.x >= 0.0f && norm.x < float(resX) &&
            norm.y >= 0.0f && norm.y < float(resX) &&
            norm.z >= 0.0f && norm.z < float(resX) &&
            triBoxOverlap(floor(norm) + 0.5f, vec3(0.5f), triverts) == 1;
        norms[x] = norm;
    }
/*
    beginInvocationInterlockNV();
    for(int x=0;x<BIAS;x++){
        if(overlap[x]){
            vec3 norm = norms[x];
            uvec2 t = searchVoxelIndex(uvec3(floor(norm)));
            if(t.x == LONGEST){
                uint i = atomicCounterIncrement(scounter);
                voxels_subgrid[t.y] = i;
                Voxel vox;
                vox.last = LONGEST;
                vox.count = 0;
                voxels[i] = vox;
            }
        }
    }
    endInvocationInterlockNV();
    memoryBarrier();
*/
    for(int x=0;x<BIAS;x++){
        if(overlap[x]){
            vec3 norm = norms[x];
            uvec2 t = searchVoxelIndex(uvec3(floor(norm)));
            atomicAdd(voxels[t.x].count, 1);
            uint i = atomicCounterIncrement(vcounter);
            Thash thash;
            thash.triangle = gl_PrimitiveID;
            thash.previous = atomicExchange(voxels[t.x].last, i);
            thashes[i] = thash;
        }
    }

    outColor = vec4(vec3(0.0f), 1.0);
}