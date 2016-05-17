#version 450 compatibility
layout(std430, binding=3) buffer s_vbo {float verts[];};
layout(std430, binding=4) buffer s_ebo {uint indics[];};
layout(std430, binding=5) buffer s_norm {float norms[];};
layout(std430, binding=6) buffer s_tex {float texcs[];};
layout(std430, binding=7) buffer s_mat {int mats[];};
out vec3 normals;
out vec2 texcoords;
out vec3 vertss;
flat out int materialID;
void main()
{
    uint udc = indics[gl_VertexID];
    vec3 position = vec3(verts[udc * 3 + 0], verts[udc * 3 + 1], verts[udc * 3 + 2]);
    gl_Position = vec4(position, 1.0);
    vertss = position;
    normals = vec3(norms[udc * 3 + 0], norms[udc * 3 + 1], norms[udc * 3 + 2]);
    texcoords = vec2(texcs[udc * 2 + 0], texcs[udc * 2 + 1]);
    materialID = mats[gl_VertexID / 3];
}