struct Light {
    vec4 directionOrPosition;
    vec3 color;
    vec4 attenuation;
    vec3 spotLightDirection;
};

#define SHADOW_STRENGTH 1
#define SHADOWMAP_SIZE 2048
#define PI 3.14159265359

float calculateShadow(Light light, sampler2D shadowMapTex, vec3 projCoords) {
    // If the fragment is outside the light's projection then it is outside
    // the light's influence, which means it is in the shadow (notice that
    // such sample would be outside the shadow map image)
    if (abs(projCoords.z) > 1.0)
        return 1.0;
    
    // Translate from NDC to shadow map space (Vulkan's Z is already in [0..1])
    vec2 shadowMapCoord = projCoords.xy * 0.5 + 0.5;
    
    // Check if the sample is in the light or in the shadow
    if (projCoords.z > texture(shadowMapTex, shadowMapCoord.xy).x)
        return 1 - SHADOW_STRENGTH;
    
    // In the light
    return 1.0;
}

float random(vec3 seed, int i) {
    vec4 seed4 = vec4(seed, i);
    float dotProduct = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dotProduct) * 43758.5453);
}

float calculateShadowPCF(Light light, vec4 lightSpacePosition, sampler2D shadowMapTex)
{
    float scale = 1.5;
    float scaleDelta = scale * 1.0 / float(SHADOWMAP_SIZE);
    
    float shadowFactor = 0.0;
    
    int range = 1;
    int count = (range*2 + 1) * (range*2 + 1);
    
    // perform perspective divide
    vec3 projCoords = lightSpacePosition.xyz / lightSpacePosition.w;
    
    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadowFactor += calculateShadow(light, shadowMapTex, projCoords + vec3(scaleDelta * x, scaleDelta * y, 0.0));
        }
    }
    
    return shadowFactor / count;
}

// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#poisson-sampling
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
);

float calculateShadowPCFPoisson(Light light, vec4 lightSpacePosition, vec3 normal, sampler2D shadowMapTex)
{
    /*ivec2 texDim = textureSize(shadowMapTex, 0);
    float scale = 1.5;
    float scaleDelta = scale * 1.0 / float(texDim.x);
    
    float shadowFactor = 0.0;
    int count = 0;
    int range = 2;
    
    int samples = 4;
    
    // perform perspective divide
    vec3 projCoords = lightSpacePosition.xyz / lightSpacePosition.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    for (int i = 0; i <= samples; i++)
    {
        int index = int(16.0 * random(gl_FragCoord.xyy, i)) % 16;
        shadowFactor += calculateShadow(light, lightSpacePosition, normal, shadowMapTex, projCoords + projCoords + vec3(poissonDisk[index] * scaleDelta * range, poissonDisk[index] * scaleDelta * range, 0.0));
        count++;
    }
    
    return shadowFactor / count;*/
    return 0.0f;
}

vec3 calculateLight(Light light, vec3 fragPos, vec3 viewPos, vec3 albedo, vec3 normal, float metallic, float smoothness, bool shadows, sampler2D shadowMapTex, vec3 geometryNormal, vec4 shadowCoord) {
    // Diffuse
    vec3 lightVector = light.directionOrPosition.xyz - fragPos * light.directionOrPosition.w;
    vec3 lightDir = normalize(lightVector);
    float distanceSqr = max(dot(lightVector, lightVector), 0.00001);
    
    float spotFade = dot(lightDir, -light.spotLightDirection);
    spotFade = clamp(spotFade * light.attenuation.z + light.attenuation.w, 0, 1);
    spotFade *= spotFade;
    
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * light.color;
    diffuse *= spotFade;
    diffuse /= distanceSqr;
    
    // Specular
    vec3 viewDir = normalize(viewPos - fragPos);
    //vec3 reflectDir = reflect(-lightDir, norm);
    vec3 halfVector = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfVector, normal), 0.0), mix(8, 64, smoothness));
    vec3 specularTint = albedo * metallic;
    
    // Energy conservation
    float oneMinusReflectivity = mix(1, 0.08, metallic);
    albedo *= oneMinusReflectivity;
    vec3 specular = spec * specularTint * light.color;
    specular *= spotFade;
    specular /= distanceSqr;
    
    return (diffuse * albedo + specular) * calculateShadowPCF(light, shadowCoord, shadowMapTex);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
