#version 450 compatibility
#extension GL_NV_gpu_shader5 : enable
in vec2 position;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}