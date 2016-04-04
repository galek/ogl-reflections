layout(std430, binding=3) buffer s_vbo {float verts[];};
layout(std430, binding=4) buffer s_ebo {uint indics[];};
layout(std430, binding=5) buffer s_norm {float norms[];};
layout(std430, binding=6) buffer s_tex {float texcs[];};
layout(std430, binding=7) buffer s_mat {int mats[];};