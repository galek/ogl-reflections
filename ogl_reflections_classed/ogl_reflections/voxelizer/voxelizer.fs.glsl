#version 450 core
#extension GL_NV_gpu_shader5 : enable
#extension GL_NV_shader_atomic_fp16_vector : enable
#extension GL_ARB_shading_language_include : require

#include </constants>
#include </uniforms>
#include </fastmath>
#include </octree_raw>

out vec4 outColor;

flat in int shortest;
in vec3 fragPosition;
flat in vec3 vert0;
flat in vec3 vert1;
flat in vec3 vert2;
flat in vec3 normal;
in int gl_PrimitiveID;

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

    for(int x=0;x<BIAS;x++){
        vec3 norm = vec3(coord.xy, z[x]);
        if(shortest == 1) {
            norm.xyz = norm.xzy;
        } else
        if(shortest == 0) {
            norm.xyz = norm.zyx;
        }

        if(
            norm.x >= 0.0f && norm.x < float(resX) &&
            norm.y >= 0.0f && norm.y < float(resX) &&
            norm.z >= 0.0f && norm.z < float(resX) &&
            triBoxOverlap(floor(norm) + 0.5f, vec3(0.5001f), triverts) == 1
        ){
            uint idx = atomicCounterIncrement(dcounter);
            uvec3 co = uvec3(floor(norm.xyz));
            VoxelRaw vox;
            vox.coordX = co.x;
            vox.coordY = co.y;
            vox.coordZ = co.z;
            vox.triangle = gl_PrimitiveID;
            voxels_raw[idx] = vox;
        }
    }
}