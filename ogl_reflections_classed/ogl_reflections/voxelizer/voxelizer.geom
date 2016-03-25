#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 verts[];

flat out vec3 vert0;
flat out vec3 vert1;
flat out vec3 vert2;
flat out vec3 normal;
flat out int shortest;
in int gl_PrimitiveIDIn;
out int gl_PrimitiveID;

uniform uint currentDepth;
uniform vec3 offset;
uniform vec3 scale;

float lengthFast(in vec3 v){
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3 normalizeFast(in vec3 v){
    return v / lengthFast(v);
}

void main(){
    vec3 positions[3];
    vec3 res = vec3(pow(2, currentDepth));

    for (int i = 0; i < 3; i++) {
        vec3 vert = verts[i];
        vec3 pos = vert;
        pos -= offset;
        pos /= scale;
        pos *= res;
        positions[i] = pos;
    }

    vec3 normals = normalizeFast(cross(positions[2] - positions[0], positions[1] - positions[0]));
    vec3 norm = abs(normals);

    vert0 = positions[0];
    vert1 = positions[1];
    vert2 = positions[2];
    normal = normals;

    vec3 mn = min(positions[0], min(positions[1], positions[2]));
    vec3 mx = max(positions[0], max(positions[1], positions[2]));
    vec3 df = mx - mn;

    int shrt = 2;
    if(norm.x > norm.z && norm.x >= norm.y) shrt = 0; else //X
    if(norm.y > norm.x && norm.y >= norm.z) shrt = 1; else //Y
    if(norm.z > norm.y && norm.z >= norm.x) shrt = 2;  //Z
    shortest = shrt;

    gl_PrimitiveID = gl_PrimitiveIDIn;

    vec3 ce = (positions[0] + positions[1] + positions[2]) / 3.0f;
    for (int i = 0; i < 3; i++) {
        vec3 pos = positions[i];
        vec3 opos = pos;

        //vec3 vect = normalizeFast(opos - ce);
        //vec3 av = abs(vect);
        //vect /= max(av.x, max(av.y, av.z));
        //opos += vect;

        opos = (opos / res) * 2.0 - 1.0; //Centroid position
        if(shrt == 2){
            gl_Position = vec4(opos.xyz, 1.0);
        } else
        if(shrt == 1){
            gl_Position = vec4(opos.xzy, 1.0);
        } else
        if(shrt == 0){
            gl_Position = vec4(opos.zyx, 1.0);
        }
        EmitVertex();
    }

    EndPrimitive();
}