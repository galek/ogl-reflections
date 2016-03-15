#version 450 core
in vec3 position;
out vec3 verts;
void main()
{
    gl_Position = vec4(position, 1.0);
    verts = position;
}