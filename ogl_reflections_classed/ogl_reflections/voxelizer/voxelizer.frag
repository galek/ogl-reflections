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

void store3(in vec3 a, inout float b[3]){
    b[0] = a.x;
    b[1] = a.y;
    b[2] = a.z;
}


#define X 0
#define Y 1
#define Z 2

#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2];
#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if(x1<min) min=x1;\
  if(x1>max) max=x1;\
  if(x2<min) min=x2;\
  if(x2>max) max=x2;

int planeBoxOverlap(float normal[3], float vert[3], float maxbox[3])
{
    int q;
    float vmin[3],vmax[3],v;
    for(q=X;q<=Z;q++){
        v=vert[q];
        if(normal[q]>0.0f){
            vmin[q]=-maxbox[q] - v;
            vmax[q]= maxbox[q] - v;
        }else{
            vmin[q]= maxbox[q] - v;
            vmax[q]=-maxbox[q] - v;
        }
    }
    if(DOT(normal,vmin)>0.0f) return 0;
    if(DOT(normal,vmax)>=0.0f) return 1;
    return 0;
}

#define AXISTEST_X01(a, b, fa, fb)			   \
	p0 = a*v0[Y] - b*v0[Z];			       	   \
	p2 = a*v2[Y] - b*v2[Z];			       	   \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
	rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_X2(a, b, fa, fb)			   \
	p0 = a*v0[Y] - b*v0[Z];			           \
	p1 = a*v1[Y] - b*v1[Z];			       	   \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Y02(a, b, fa, fb)			   \
	p0 = -a*v0[X] + b*v0[Z];		      	   \
	p2 = -a*v2[X] + b*v2[Z];	       	       	   \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
	rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Y1(a, b, fa, fb)			   \
	p0 = -a*v0[X] + b*v0[Z];		      	   \
	p1 = -a*v1[X] + b*v1[Z];	     	       	   \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Z12(a, b, fa, fb)			   \
	p1 = a*v1[X] - b*v1[Y];			           \
	p2 = a*v2[X] - b*v2[Y];			       	   \
        if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
	rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Z0(a, b, fa, fb)			   \
	p0 = a*v0[X] - b*v0[Y];				   \
	p1 = a*v1[X] - b*v1[Y];			           \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
	if(min>rad || max<-rad) return 0;

int triBoxOverlap(float boxcenter[3],float boxhalfsize[3],float triverts[3][3]){
   float v0[3],v1[3],v2[3];
   float min,max,p0,p1,p2,rad,fex,fey,fez;
   float normal[3],e0[3],e1[3],e2[3];

   SUB(v0,triverts[0],boxcenter);
   SUB(v1,triverts[1],boxcenter);
   SUB(v2,triverts[2],boxcenter);

   SUB(e0,v1,v0);
   SUB(e1,v2,v1);
   SUB(e2,v0,v2);

   fex = abs(e0[X]);
   fey = abs(e0[Y]);
   fez = abs(e0[Z]);

   AXISTEST_X01(e0[Z], e0[Y], fez, fey);
   AXISTEST_Y02(e0[Z], e0[X], fez, fex);
   AXISTEST_Z12(e0[Y], e0[X], fey, fex);

   fex = abs(e1[X]);
   fey = abs(e1[Y]);
   fez = abs(e1[Z]);

   AXISTEST_X01(e1[Z], e1[Y], fez, fey);
   AXISTEST_Y02(e1[Z], e1[X], fez, fex);
   AXISTEST_Z0(e1[Y], e1[X], fey, fex);

   fex = abs(e2[X]);
   fey = abs(e2[Y]);
   fez = abs(e2[Z]);

   AXISTEST_X2(e2[Z], e2[Y], fez, fey);
   AXISTEST_Y1(e2[Z], e2[X], fez, fex);
   AXISTEST_Z12(e2[Y], e2[X], fey, fex);

   FINDMINMAX(v0[X],v1[X],v2[X],min,max);
   if(min>boxhalfsize[X] || max<-boxhalfsize[X]) return 0;

   FINDMINMAX(v0[Y],v1[Y],v2[Y],min,max);
   if(min>boxhalfsize[Y] || max<-boxhalfsize[Y]) return 0;

   FINDMINMAX(v0[Z],v1[Z],v2[Z],min,max);
   if(min>boxhalfsize[Z] || max<-boxhalfsize[Z]) return 0;

   CROSS(normal,e0,e1);
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

    float triverts[3][3];
    store3(vert0, triverts[0]);
    store3(vert1, triverts[1]);
    store3(vert2, triverts[2]);

    float bhalfsize[3];
    store3(vec3(0.5), bhalfsize);

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

        float bcenter[3]; store3(norm, bcenter);
        overlap[x] =
            norm.x >= 0.0f && norm.x < float(resX) &&
            norm.y >= 0.0f && norm.y < float(resX) &&
            norm.z >= 0.0f && norm.z < float(resX) &&
            triBoxOverlap(bcenter, bhalfsize, triverts) == 1;
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