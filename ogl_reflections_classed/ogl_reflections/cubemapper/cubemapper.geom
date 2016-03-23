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

void main(){
    gl_PrimitiveID = gl_PrimitiveIDIn;

    for(int j = 0; j < 6; ++j) {
        face = j;
        for(int i = 0; i < 3; ++i) {
            position = verts[i];
            gl_Position = proj * cams[j] * vec4(verts[i] - center, 1.0f);
            EmitVertex();
        }
        EndPrimitive();
    }
}