#version 430 core

struct Light {
    int type;
    float intensity;
    vec3 position;
    vec3 direction;
    float radius;
    vec3 color;
    float innerCutoff;
    float outerCutoff;
    bool castShadow;
};

layout(std430, binding = 0) buffer LightBuffer {
    Light lights[];
};

out vec4 fragColor;

in vec2 texCoord;
in vec4 color;
in vec3 worldPos;
in mat4 viewMatrix;
in vec3 T;
in vec3 B;
in vec3 N;

uniform int lightNB;

uniform vec3 camPos;

uniform bool masked;

// Shadow maps

#define MAX_DIRECTIONAL_LIGHTS 3
#define CASCADES_PER_LIGHT 3
#define NUM_CASCADES (MAX_DIRECTIONAL_LIGHTS * CASCADES_PER_LIGHT)

uniform sampler2DShadow shadow_dirShadowMaps[NUM_CASCADES];
uniform mat4 shadow_dirLightSpaceMatrices[NUM_CASCADES];
uniform float shadow_cascadeSplits[NUM_CASCADES];

uniform sampler2DShadow shadow_spotShadowMaps[10];
uniform mat4 shadow_spotLightSpaceMatrices[10];

uniform samplerCubeArrayShadow shadow_pointShadowMapArray;
uniform float shadow_pointLightFarPlanes[10];

// PBR values
const float PI = 3.141592653589793;
uniform sampler2D albedo;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform float metallic;
uniform float roughness;
uniform float ambientIntensity;

vec3 gridSamplingDisk[20] = vec3[](
    vec3(1,1,1), vec3(1,-1,1), vec3(-1,-1,1), vec3(-1,1,1), 
    vec3(1,1,-1), vec3(1,-1,-1), vec3(-1,-1,-1), vec3(-1,1,-1),
    vec3(1,1,0), vec3(1,-1,0), vec3(-1,-1,0), vec3(-1,1,0),
    vec3(1,0,1), vec3(-1,0,1), vec3(1,0,-1), vec3(-1,0,-1),
    vec3(0,1,1), vec3(0,-1,1), vec3(0,-1,-1), vec3(0,1,-1)
);

// IBL

uniform samplerCube ibl_irradianceMap;          // diffuse IBL
uniform samplerCube ibl_prefilteredEnvMap;      // specular mipmapped env map
uniform sampler2D   ibl_brdfLUT;                // 2D BRDF integration LUT

uniform bool useEnvReflections;

///////// Shadow Functions /////////

// Cascade Selection
int selectCascade(int lightIndex){
    vec4 viewPos = viewMatrix * vec4(worldPos, 1.0);
    float depth = abs(viewPos.z);

    int base = lightIndex * CASCADES_PER_LIGHT;

    for (int i = 0; i < CASCADES_PER_LIGHT; ++i) {
        if (depth < shadow_cascadeSplits[base + i])
            return base + i;
    }

    return base + CASCADES_PER_LIGHT - 1;
}

float ShadowCalculationCSM(
    sampler2DShadow shadowMap,
    mat4 lightSpaceMatrix,
    vec3 lightDir,
    vec3 worldPos,
    vec3 worldNormal)
{
    vec4 fragLS = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 proj = fragLS.xyz / fragLS.w;

    // Transform to [0..1]
    proj = proj * 0.5 + 0.5;

    // Outside light frustum → not shadowed
    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0 ||
        proj.z > 1.0)
        return 0.0;

    // ----- Slope-scale bias -----
    float bias = max(0.0005 * (1.0 - dot(normalize(worldNormal), -lightDir)), 0.00005);

    // ----- PCF -----
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            // sampler2DShadow expects vec3 (uv, depth)
            shadow += texture(shadowMap, vec3(proj.xy + offset, proj.z - bias));
        }
    }

    shadow /= 25.0;

    return 1.0 - shadow; // return shadow factor (0..1)
}

float ShadowCalculation(sampler2DShadow shadowMap, vec3 lightDir, mat4 lightSpaceMatrix, vec3 worldNormal) {
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0)
        return 0.0;

    float bias = max(0.005 * (1.0 - dot(normalize(worldNormal), lightDir)), 0.002);

    float shadow = 0.0;
    float texelSize = 1.0 / textureSize(shadowMap, 0).x;

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z - bias));
        }
    }

    shadow /= 25.0;

    return 1.0 - shadow;
}

float ShadowPointArray(int index, vec3 lightPos, vec3 fragPos, float farPlane) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight) / farPlane;  // Normalize depth to [0..1]

    float shadow = 0.0;
    float bias = 0.05;

    // Calculate viewDistance for adaptive radius
    float viewDistance = length(camPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / farPlane)) / 25.0;

    for (int i = 0; i < 20; ++i) {
        vec3 sampleOffset = normalize(fragToLight) + gridSamplingDisk[i] * diskRadius;

        // The vec4: xyz = direction vector (normalized), w = layer index
        float visibility = texture(shadow_pointShadowMapArray, vec4(sampleOffset, float(index)), currentDepth - bias);

        shadow += visibility;
    }

    return shadow / 20.0;
}

///////// Lighting /////////

vec3 IBL_Diffuse(vec3 N, vec3 albedo, float metallic) {
    vec3 kS = mix(vec3(0.04), albedo, metallic);
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 irradiance = texture(ibl_irradianceMap, N).rgb;
    return irradiance * albedo * kD;
}

vec3 IBL_Specular(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    vec3 R = reflect(-V, N);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Sample BRDF LUT (NdotV, roughness)
    float NdotV = max(dot(N, V), 0.0);
    vec2 brdf = texture(ibl_brdfLUT, vec2(NdotV, roughness)).rg;

    // Sample prefiltered env map using roughness mip level
    const float MAX_REFLECTION_LOD = 5.0;   // depends on your cube map mip count
    vec3 prefilteredColor = textureLod(ibl_prefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD).rgb;

    // Fresnel-Schlick for IBL  
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

    return prefilteredColor * (F * brdf.x + brdf.y);
}

vec3 computeLightDisney(Light light, vec3 L, vec3 V, vec3 N, vec3 baseColor, float roughness, float metallic, float shadow, float attenuation) {
    vec3 Nn = normalize(N);
    vec3 Ln = normalize(L);
    vec3 Vn = normalize(V);
    vec3 H = normalize(Vn + Ln);

    float NdotL = max(dot(Nn, Ln), 0.0);
    float NdotV = max(dot(Nn, Vn), 0.0);
    float NdotH = max(dot(Nn, H), 0.0);
    float VdotH = max(dot(Vn, H), 0.0);

    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    // GGX Distribution
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    float D = a2 / (PI * denom * denom);

    // Geometry
    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    float G_V = NdotV / (NdotV * (1.0 - k) + k);
    float G_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G_V * G_L;

    // Fresnel
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    // Specular BRDF
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    // Diffuse term (energy conserving)
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    vec3 diffuse = kD * baseColor / 3.14159265359;

    // Combine with light color, intensity, NdotL, shadow, attenuation
    vec3 Lo = (diffuse + specular) * NdotL * light.color * light.intensity * (1.0 - shadow) * attenuation;

    return Lo;
}

void main() {
    vec4 baseColor = texture(albedo, texCoord);

    if (masked && baseColor.a < 0.5)
        discard;

    float metallicValue = texture(metallicMap, texCoord).r * metallic;
    float roughnessValue = texture(roughnessMap, texCoord).r * roughness;

    vec3 normalFromMap = texture(normalMap, texCoord).rgb;
    normalFromMap = normalFromMap * 2.0 - 1.0;

    mat3 TBN = mat3(T, B, N);
    vec3 worldNormal = normalize(TBN * normalFromMap);

    vec3 V = normalize(camPos - worldPos);
    vec3 result = vec3(0.0);

    int dirIdx = 0, pointIdx = 0, spotIdx = 0;

    for (int i = 0; i < lightNB; ++i) {
        Light l = lights[i];

        float shadow = 1.0;
        float attenuation = 1.0;
        vec3 L = vec3(0.0);

        if (l.type == 0) { // Directional
            L = -normalize(l.direction);
            if (l.castShadow) {
                int cascadeIdx = selectCascade(dirIdx);
                shadow = ShadowCalculationCSM(shadow_dirShadowMaps[cascadeIdx], shadow_dirLightSpaceMatrices[cascadeIdx], L, worldPos, worldNormal);
                dirIdx++;
            }
        }
        else if (l.type == 1) { // Point
            vec3 toLight = l.position - worldPos;
            float dist = length(toLight);
            L = normalize(toLight);

            attenuation = clamp(1.0 - (dist / l.radius), 0.0, 1.0);
            attenuation *= attenuation;

            if (l.castShadow){
                shadow = ShadowPointArray(pointIdx, l.position, worldPos, shadow_pointLightFarPlanes[pointIdx]);
                pointIdx++;
            }
        }
        else if (l.type == 2) { // Spot
            
            vec3 toFrag = worldPos - l.position;


            float dist = length(toFrag);
            if (dist > l.radius)
                continue;

            L = normalize(l.position - worldPos);

            float angleFactor = dot(normalize(l.direction), normalize(toFrag));

            float spotEffect = clamp((angleFactor - l.outerCutoff) / (l.innerCutoff - l.outerCutoff), 0.0, 1.0);

            attenuation = clamp(1.0 - (dist / l.radius), 0.0, 1.0);
            attenuation *= attenuation;
            attenuation *= spotEffect;

            if (l.castShadow) {
                shadow = ShadowCalculation(shadow_spotShadowMaps[spotIdx], l.direction, shadow_spotLightSpaceMatrices[spotIdx], worldNormal);
                spotIdx++;
            }

        }

        result += computeLightDisney(l, L, V, worldNormal, baseColor.rgb, roughnessValue, metallicValue, shadow, attenuation);
    }


    if(useEnvReflections){
        vec3 specularIBL = IBL_Specular(worldNormal, V, baseColor.rgb, metallicValue, roughnessValue);
        vec3 diffuseIBL = IBL_Diffuse(worldNormal, baseColor.rgb, metallicValue);
        vec3 lighting = result + diffuseIBL + specularIBL;
        fragColor = vec4(lighting, baseColor.a);
    }
    else{
        vec4 color = baseColor * ambientIntensity + vec4(result, baseColor.a);
        fragColor = color;
    }
}