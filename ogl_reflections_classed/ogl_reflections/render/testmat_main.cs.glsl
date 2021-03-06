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

uniform vec3 light;
uniform float reflectivity;
uniform float dissolve;
uniform float transmission;
uniform uint materialID;
uniform float illumPower;

uniform sampler2D tex;
uniform sampler2D bump;
uniform sampler2D spec;
uniform sampler2D illum;

vec3 getNormalMapping(vec2 texcoordi){
    vec2 textures = textureSize(bump, 0);
    vec2 texcoord = texcoordi;//(floor(texcoordi * textures) + 0.5f) / textures;
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

                Ray newRay = ray;
                newRay.params.w = 1.0f;
                newRay.origin += newRay.direct * hit.dist;

                vec3 binormal = normalizeFast(cross(hit.tangent, hit.normal));
                mat3 tbn = transpose(mat3(hit.tangent, binormal, hit.normal));

                vec3 prenormal = hit.normal;
                prenormal = normalizeFast(getNormalMapping(hit.texcoord) * tbn);

                vec3 normal = -normalizeFast(prenormal * dot(prenormal, newRay.direct));
                vec3 color = tx.rgb;

                float iorIn = 1.0f;
                float iorOut = 1.6666f;

                bool cond = dot(prenormal, newRay.direct) <= 0;
                float coef = cond ? iorIn / iorOut : iorOut / iorIn;

                vec3 refl = normalizeFast(mate(newRay.direct, normal, reflectivity));
                vec3 refr = normalizeFast(refract(toDir(refl, normal), normal, coef));

                float fres = computeFresnel(
                    normal,
                    newRay.direct,
                    refr,
                    cond ? iorIn : iorOut,
                    cond ? iorOut : iorIn
                );

                tx.a *= dissolve >= 0.0f ? clamp(dissolve, 0.02f, 1.0f) : clamp(-dissolve, 0.02f, 1.0f);
                if(tx.a >= 0.999f){
                //    tx.a *= dot(newRay.direct, prenormal) <= 0.0f ? 1.0f : 0.0f;
                }
                if(tx.a >= 0.001f){
                    vec4 ilTx = texture(illum, hit.texcoord);
                    vec3 ilColor = ilTx.rgb;
                    float ipa = ilTx.a;
                    if(ipa >= 0.001f){
                        Ray newRay2 = newRay;
                        newRay2.direct = refr;
                        newRay2.color *= ipa;
                        newRay2.params0.y *= ipa;
                        newRay2.final = newRay2.color * ilColor * illumPower;
                        newRay2.params.y += 1.0f;
                        newRay2.actived = 0;
                        newRay2.origin += normalizeFast(normal * dot(newRay2.direct, normal)) * 0.1f;
                        uint idx = createRay(newRay2);
                        storeHit(rays[idx], hit);
                        storeTexel(ray.texel, idx);
                    }

                    vec4 specTx = texture(spec, hit.texcoord);
                    float spc = 1.0f;
                    float spl = length(specTx.rgb);
                    spc *= spl;
                    spc *= 1.0f - ipa;
                    spc *= mix(1.0f, fres, transmission);
                    spc = clamp(spc, 0.0f, 1.0f);

                    if(spc >= 0.001f){
                        Ray newRay2 = newRay;
                        newRay2.direct = refl;
                        newRay2.color *= 1.0f / spl * spc * specTx.rgb;
                        newRay2.params0.y *= spc;
                        newRay2.params.y += 1.0f;
                        newRay2.origin += normalizeFast(normal * dot(newRay2.direct, normal)) * 0.1f;
                        uint idx = createRay(newRay2);
                        storeHit(rays[idx], hit);
                        storeTexel(ray.texel, idx);
                    }

                    float samp = 1.0f;
                    samp *= clamp(1.0f - ipa, 0.0f, 1.0f);
                    samp *= clamp(1.0f - spc, 0.0f, 1.0f);
                    samp *= clamp(1.0f / DIFFUSE_COUNT * tx.a, 0.0f, 1.0f);
                    if(samp >= 0.001f){
                        for(int i=0;i<DIFFUSE_COUNT;i++){
                            Ray newRay2 = newRay;
                            newRay2.direct = randomCosine(normal);
                            newRay2.color *= color * samp;
                            newRay2.params0.y *= samp;
                            newRay2.params.y += 1.0f;
                            newRay2.origin += normalizeFast(normal * dot(newRay2.direct, normal)) * 0.1f;
                            uint idx = createRay(newRay2);
                            storeHit(rays[idx], hit);
                            storeTexel(ray.texel, idx);
                        }
                    }

                    float ref = 1.0f;
                    ref *= clamp(1.0f - ipa, 0.0f, 1.0f);
                    ref *= clamp(1.0f - tx.a, 0.0f, 1.0f);
                    ref *= clamp(1.0f - spc, 0.0f, 1.0f);
                    if(ref >= 0.001f){
                        Ray newRay2 = newRay;
                        newRay2.direct = refr;
                        newRay2.color *= ref;
                        newRay2.params0.y *= ref;
                        newRay2.params.y += 1.0f;
                        newRay2.origin += normalizeFast(normal * dot(newRay2.direct, normal)) * 0.1f;
                        uint idx = createRay(newRay2);
                        storeHit(rays[idx], hit);
                        storeTexel(ray.texel, idx);
                    }

                    newRay.actived = 0;
                    newRay.params0.x = 0.0f;
                }

                newRay.color *= 1.0f - tx.a;
                newRay.params0.y *= 1.0f - tx.a;
                newRay.origin += normalizeFast(normal * dot(newRay.direct, normal)) * 0.1f;
                storeRay(t, newRay);
            }
        }
    }
}
