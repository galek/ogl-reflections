#version 450 compatibility
#extension GL_ARB_shading_language_include : require
#extension GL_NV_gpu_shader5 : enable
#include </constants>
#include </uniforms>
#include </fastmath>
#include </structs>
#include </random>
#define DIFFUSE_COUNT 1

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

uniform uint materialID;
uniform vec3 light;
uniform float reflectivity;
uniform float dissolve;
uniform float illumPower;
uniform float ior;

uniform sampler2D tex;
uniform sampler2D bump;
uniform sampler2D spec;
uniform sampler2D illum;
uniform sampler2D trans;

#define NOISE_DENSITY 4.0f

vec3 getNormalMapping(vec2 texcoordi){
    vec2 textures = textureSize(bump, 0);
    vec2 texcoord = texcoordi * NOISE_DENSITY;//(floor(texcoordi * textures) + 0.5f) / textures;
    float val = texture(bump, texcoord).r;
    float valX = texture(bump, texcoord + vec2(1.0f / textures.x, 0.0f) * 1.0f).r;
    float valY = texture(bump, texcoord + vec2(0.0f, 1.0f / textures.y) * 1.0f).r;
    vec3 h = vec3(1.0f,0.0f,val - valX);
    vec3 v = vec3(0.0f,1.0f,val - valY);
    return normalizeFast(cross(h,v));
}

float computeFresnel(in vec3 normal, in vec3 incident, in vec3 transmissionDirection, in float refractiveIndexIncident, in float refractiveIndexTransmitted) {
	float cosThetaIncident = dot(normal, incident);
	float cosThetaTransmitted = dot(normal, transmissionDirection);
	float reflectionCoefficientSPolarized = pow((refractiveIndexIncident * cosThetaIncident - refractiveIndexTransmitted * cosThetaTransmitted)   /   (refractiveIndexIncident * cosThetaIncident + refractiveIndexTransmitted * cosThetaTransmitted)   , 2);
    float reflectionCoefficientPPolarized = pow((refractiveIndexIncident * cosThetaTransmitted - refractiveIndexTransmitted * cosThetaIncident)   /   (refractiveIndexIncident * cosThetaTransmitted + refractiveIndexTransmitted * cosThetaIncident)   , 2);
	float reflectionCoefficientUnpolarized = (reflectionCoefficientSPolarized + reflectionCoefficientPPolarized) / 2.0; // Equal mix.
	return clamp(reflectionCoefficientUnpolarized, 0.0f, 1.0f);
}

vec3 toDir(in vec3 refl, in vec3 normal){
    return -reflect(-refl, normal);
}

vec3 glossy(in vec3 dir, in vec3 normal, in float refli){
    return mix(normalizeFast(reflect(dir, normal)), normalizeFast(randomCosine(normal)), clamp(sqrt(random()) * refli, 0.0f, 1.0f));
}

vec3 mate(in vec3 dir, in vec3 normal, in float refli){
    return mix(normalizeFast(randomCosine(normal)), normalizeFast(reflect(dir, normal)), clamp(sqrt(random()) * refli, 0.0f, 1.0f));
}

void main()
{
    if(gl_GlobalInvocationID.x < rayCount){
        uint t = gl_GlobalInvocationID.x;
        Ray ray = rays[t];
        if(ray.actived > 0 && ray.params.w < 1.0f && ray.params.y < ray.params.z){
            Hit hit = hits[ray.hit];
            if(hit.materialID == materialID && hit.dist < 10000.0f){
                vec4 tx = texture(tex, hit.texcoord);
                vec4 sx = texture(spec, hit.texcoord);
                vec4 ttx = texture(trans, hit.texcoord);
                vec4 itx = texture2D(illum, hit.texcoord);

                Ray newRay = ray;
                newRay.params.w = 1.0f;
                newRay.origin += newRay.direct * hit.dist;

                vec3 binormal = normalizeFast(cross(hit.tangent, hit.normal));
                mat3 tbn = transpose(mat3(hit.tangent, binormal, hit.normal));

                vec3 prenormal = hit.normal;
                prenormal = normalizeFast(getNormalMapping(hit.texcoord) * tbn);
                float cos_ray = dot(prenormal, newRay.direct);
                bool cond = cos_ray < 0.0f;
                vec3 normal = -normalizeFast(prenormal * cos_ray);

                float iorIn = 1.0f;
                float iorOut = ior;
                float coef = cond ? iorIn / iorOut : iorOut / iorIn;

                float shiny = reflectivity;
                vec3 refl = normalizeFast(mate(newRay.direct, normal, shiny));
                vec3 refr = normalizeFast(refract(toDir(refl, normal), normal, coef));

                float fres = computeFresnel(
                    normal,
                    newRay.direct,
                    refr,
                    cond ? iorIn : iorOut,
                    cond ? iorOut : iorIn
                );
                
                vec3 icolor = itx.rgb;
                float ilen = length(icolor);
                
                if(random() <= tx.a){
                    if(random() <= illumPower){
                        newRay.final = newRay.color * icolor;
                        newRay.actived = 0;
                    } else 
                    if(random() > fres){
                        if(random() <= dissolve){
                            vec3 dir = randomCosine(normal);
                            if(dot(dir, normal) > 0.0f && cond){
                                newRay.direct = randomCosine(normal);
                                newRay.color *= tx.rgb;
                            }
                        } else {
                            if(dot(refr, normal) < 0.0f){
                                newRay.direct = refr;
                                newRay.color *= ttx.rgb;
                            }
                        }
                    } else 
                    if(dot(refl, normal) > 0.0f && cond){
                        newRay.direct = refl;
                        newRay.color *= sx.rgb;
                    }
                }
                
                newRay.origin += normalizeFast(normal * dot(newRay.direct, normal)) * 0.1f;
                storeRay(t, newRay);
            }
        }
    }
}