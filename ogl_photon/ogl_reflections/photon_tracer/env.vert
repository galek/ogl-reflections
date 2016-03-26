#version 450 core
in vec2 position;
out vec3 verts;
void main()
{
    gl_Position = vec4(position, 0.0f, 1.0f);
    verts = vec3(position, 0.0f);
}