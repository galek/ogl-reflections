#version 450 compatibility

layout(triangles) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 vertss[];
in vec2 texcoords[];
in vec3 normals[];
flat in int materialID[];

flat out vec3 vert0;
flat out vec3 vert1;
flat out vec3 vert2;
flat out vec3 normal0;
flat out vec3 normal1;
flat out vec3 normal2;
flat out vec2 texcoord0;
flat out vec2 texcoord1;
flat out vec2 texcoord2;
flat out int materialIDp;
in int gl_PrimitiveIDIn;
out int gl_PrimitiveID;

void main(){
    materialIDp = materialID[0];

    vert0 = vertss[0];
    vert1 = vertss[1];
    vert2 = vertss[2];

    texcoord0 = texcoords[0];
    texcoord1 = texcoords[1];
    texcoord2 = texcoords[2];
    
    normal0 = normals[0];
    normal1 = normals[1];
    normal2 = normals[2];
    
    gl_PrimitiveID = gl_PrimitiveIDIn;

    gl_Position = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
    EmitVertex();

    gl_Position = vec4(1.0f, -1.0f, 0.0f, 1.0f);
    EmitVertex();
    
    gl_Position = vec4(-1.0f, 1.0f, 0.0f, 1.0f);
    EmitVertex();
    
    gl_Position = vec4(1.0f, 1.0f, 0.0f, 1.0f);
    EmitVertex();

    EndPrimitive();
}