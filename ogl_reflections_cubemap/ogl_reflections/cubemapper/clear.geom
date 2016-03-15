#version 450 core

layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 18) out;

in vec2 verts[];
in int gl_PrimitiveIDIn;

flat out int face;
flat out vec3 vert0;
flat out vec3 vert1;
flat out vec3 vert2;
out int gl_PrimitiveID;

void main(){
    gl_PrimitiveID = gl_PrimitiveIDIn;

    for(int j = 0; j < 6; ++j) {
        face = j;
        for(int i = 0; i < 3; ++i) {
            gl_Position = vec4(verts[i], 0.0, 1.0f);
            EmitVertex();
        }
        EndPrimitive();
    }
}