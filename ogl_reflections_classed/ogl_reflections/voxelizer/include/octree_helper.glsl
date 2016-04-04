layout(std430, binding=5) buffer s_to_helper {uint to_helper[];};
layout(std430, binding=6) buffer s_from_helper {uint from_helper[];};
layout (binding=2) uniform atomic_uint lscounter_to;
layout (binding=3) uniform atomic_uint lscounter_from;