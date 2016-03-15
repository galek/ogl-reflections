#version 450 core
in vec2 position;
out vec2 verts;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    verts = position;
}