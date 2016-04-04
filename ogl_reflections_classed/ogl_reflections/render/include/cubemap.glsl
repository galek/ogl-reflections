vec3 generate_cubemap_coord(in vec2 txc, in int face)
{
    vec3 v;
    switch(face)
    {
        case 0: v = vec3( 1.0, -txc.y, -txc.x); break; // +X
        case 1: v = vec3(-1.0, -txc.y,  txc.x); break; // -X
        case 2: v = vec3( txc.x,  1.0,  txc.y); break; // +Y
        case 3: v = vec3( txc.x, -1.0, -txc.y); break; // -Y
        case 4: v = vec3( txc.x, -txc.y,  1.0); break; // +Z
        case 5: v = vec3(-txc.x, -txc.y, -1.0); break; // -Z
    }
    return normalizeFast(v);
}

vec3 get_coord(in vec3 v)
{
    const float d45 = 0.9999f;//0.70710678118f;
    float mx = max(abs(v.x), max(abs(v.y), abs(v.z)));
    vec3 vv = v / mx;
    vec3 vn = normalizeFast(v);
    vec3 f;
    if(dot(vv, vec3( 1.0,  0.0, 0.0)) > d45) f = vec3( -vv.z, -vv.y, 0.0f);
    if(dot(vv, vec3(-1.0,  0.0, 0.0)) > d45) f = vec3(  vv.z,  -vv.y, 1.0f);
    if(dot(vv, vec3( 0.0,  1.0, 0.0)) > d45) f = vec3(  vv.x,  vv.z, 2.0f);
    if(dot(vv, vec3( 0.0, -1.0, 0.0)) > d45) f = vec3(  vv.x,  -vv.z, 3.0f);
    if(dot(vv, vec3( 0.0,  0.0, 1.0)) > d45) f = vec3(   vv.x, -vv.y, 4.0f);
    if(dot(vv, vec3( 0.0,  0.0, -1.0)) > d45) f = vec3( -vv.x, -vv.y, 5.0f);
    return vec3(f.xy * 0.5 + 0.5, f.z);
}