#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "lighting.glsl"

const int MAX_LIGHTS = 4;

layout(binding = 0) uniform LocalTransform {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpace;
} t;

layout(binding = 1) uniform Globals {
    vec3 viewPos;
    vec3 ambientColor;
    Light[MAX_LIGHTS] lights;
    float normalIntensity;
} globals;

layout(binding = 2) uniform sampler2D albedoTex;
layout(binding = 3) uniform sampler2D maskTex;
layout(binding = 4) uniform sampler2D normalMapTex;
layout(binding = 5) uniform sampler2D shadowMapTex;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in mat3 fragTbn;
layout(location = 7) in vec4 fragShadowCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = texture(albedoTex, fragTexCoord).rgb;
    vec4 mgao = texture(maskTex, fragTexCoord);
    vec3 normal = texture(normalMapTex, fragTexCoord).rgb;
    
    normal = normal * 2.0 - 1.0;
    normal.z = normal.z * (1/globals.normalIntensity);
    normal = normalize(fragTbn * normal);
    
    float metallic = mgao.r;
    float roughness = 1 - mgao.g;
    float ao = mgao.b;
    
    vec3 V = normalize(globals.viewPos - fragPos);
    
    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < MAX_LIGHTS; ++i)
    {
        Light light = globals.lights[i];
        
        vec3 lightVector = light.directionOrPosition.xyz - fragPos * light.directionOrPosition.w;
        // calculate per-light radiance
        vec3 L = normalize(lightVector);
        vec3 H = normalize(V + L);
        float distanceSqr = max(dot(lightVector, lightVector), 0.00001);
        
        // float attenuation = 1.0 / distanceSqr;
        //float spotFade = dot(L, -light.spotLightDirection);
        // TODO: add light range as light.attenuation.x
        //spotFade = clamp(spotFade * light.attenuation.z + light.attenuation.w, 0, 1);
        //spotFade *= spotFade;
        
        vec3 radiance = light.color; // * attenuation * spotFade;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(normal, H, roughness);
        float G   = GeometrySmith(normal, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 nominator    = NDF * G * F;
        float denominator = 4 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
        vec3 specular = nominator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;
        
        // scale light by NdotL
        float NdotL = max(dot(normal, L), 0.0);
        
        // add to outgoing radiance Lo
        Lo += ((kD * albedo / PI + specular) * radiance * NdotL) * calculateShadowPCF(light, fragShadowCoord, shadowMapTex);  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }
    
    vec3 ambient = globals.ambientColor * albedo * ao;
    vec3 color = ambient + Lo;
    
    outColor = vec4(color, 1.0);
}
