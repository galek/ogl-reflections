float lengthFast(in vec3 v){
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3 normalizeFast(in vec3 v){
    return v / lengthFast(v);
}