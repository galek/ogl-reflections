#version 450 core
out vec3 verts;
layout(std430, binding=3) buffer s_vbo {float sverts[];};
layout(std430, binding=4) buffer s_ebo {uint indics[];};
void main()
{
    uint udc = indics[gl_VertexID];
    vec3 position = vec3(sverts[udc * 3 + 0], sverts[udc * 3 + 1], sverts[udc * 3 + 2]);
    gl_Position = vec4(position, 1.0);
    verts = position;
}