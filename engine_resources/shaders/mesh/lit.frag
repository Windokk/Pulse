#version 430 core

struct Light {
    vec4 position;
    vec4 direction;
    vec4 color;

    float intensity;
    float radius;
    float innerCutoff;
    float outerCutoff;

    int type;
    int castShadow;
    vec2 padding;
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

uniform mat4 dirLightSpaceMatrices[NUM_CASCADES];
uniform float dirCascadeSplits[NUM_CASCADES];

uniform mat4 spotLightSpaceMatrices[10];

uniform float pointLightFarPlanes[10];

layout(binding = 0) uniform sampler2D albedo;
layout(binding = 1) uniform sampler2D metallicMap;
layout(binding = 2) uniform sampler2D roughnessMap;
layout(binding = 3) uniform sampler2D normalMap;

layout(binding = 4) uniform samplerCube ibl_irradianceMap;
layout(binding = 5) uniform samplerCube ibl_prefilteredEnvMap;
layout(binding = 6) uniform sampler2D ibl_brdfLUT;

// Real-time diffuse GI (see ProbeManager) - falls back to the static IBL diffuse term above when no
// probe volume is active in the level (ddgi_enabled == false).
layout(binding = 7) uniform sampler2D ddgi_irradianceAtlas;
// Per-probe (mean hit distance, mean hit distance^2) atlas, same octahedral layout as the irradiance
// one - used by DDGI_VisibilityWeight below to stop probes that are occluded from a shading point (e.g.
// on the far side of a wall) from leaking light/shadow into it via the trilinear blend.
layout(binding = 8) uniform sampler2D ddgi_distanceAtlas;
uniform bool ddgi_enabled;
uniform vec3 ddgi_gridOrigin;
uniform vec3 ddgi_gridSpacing;
uniform vec3 ddgi_probeCounts; // ivec3 stored as vec3 - no ivec3 uniform setter on the Shader interface
uniform int ddgi_tileSize;
uniform int ddgi_atlasProbesPerRow;
uniform int ddgi_atlasSize;

layout(binding = 10) uniform sampler2DShadow dirShadowMaps[NUM_CASCADES];
layout(binding = 20) uniform sampler2D spotShadowMaps[10];

layout(binding = 30) uniform samplerCubeArray pointShadowMapArray;

// PBR values
const float PI = 3.141592653589793;
uniform float metallic;
uniform float roughness;
uniform float ambientIntensity;

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

uniform bool useEnvReflections;

///////// Shadow Functions /////////

// Cascade Selection
int selectCascade(int lightIndex){
    vec4 viewPos = viewMatrix * vec4(worldPos, 1.0);
    float depth = abs(viewPos.z);

    int base = lightIndex * CASCADES_PER_LIGHT;

    for (int i = 0; i < CASCADES_PER_LIGHT; ++i) {
        if (depth < dirCascadeSplits[base + i])
            return base + i;
    }

    return base + CASCADES_PER_LIGHT - 1;
}

float ShadowCalculationDir(sampler2DShadow shadowMap, mat4 lightSpaceMatrix, vec3 lightDir, vec3 worldPos, vec3 worldNormal){
    vec4 fragLS = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 proj = fragLS.xyz / fragLS.w;

    // Transform to [0..1]
    proj = proj * 0.5 + 0.5;

    // Outside light frustum : not shadowed
    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0 ||
        proj.z > 1.0)
        return 1.0;

    // ----- Slope-scale bias -----
    float bias = max(0.0005 * (1.0 - dot(normalize(worldNormal), lightDir)), 0.00005);

    // ----- PCF -----
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    float visibility = 0.0; 

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            visibility += texture(shadowMap, vec3(proj.xy + offset, proj.z - bias));
        }
    }

    return visibility / 25.0;
}

float ShadowCalculationSpot(sampler2D shadowMap, vec3 lightDir, mat4 lightSpaceMatrix, vec3 worldNormal) {
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Add XY bounds check like directional
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0 || projCoords.z < 0.0)
        return 1.0;

    float bias = max(0.0005 * (1.0 - dot(normalize(worldNormal), lightDir)), 0.00005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias) < closestDepth ? 1.0 : 0.0;
        }
    }

    return shadow / 25.0;
}

float ShadowCalculationPoint(int index,vec3 lightPos,vec3 fragPos,float farPlane,vec3 worldNormal){
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias   = 0.15;
    int samples  = 20;
    float viewDistance = length(camPos - fragPos);
    float diskRadius = 0.05;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(pointShadowMapArray, vec4(fragToLight + sampleOffsetDirections[i] * diskRadius, index)).r;
        closestDepth *= farPlane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    return 1.0 - shadow;
}

///////// Lighting /////////

vec3 IBL_Diffuse(vec3 N, vec3 albedo, float metallic) {
    vec3 kS = mix(vec3(0.04), albedo, metallic);
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 irradiance = texture(ibl_irradianceMap, N).rgb;
    return irradiance * albedo * kD;
}

// Octahedral encode - inverse of the OctDecode used in probe_trace.comp to pick each probe ray's
// direction, so a texel sampled here with a given N matches the ray that was traced in that direction.
vec2 DDGI_OctEncode(vec3 n) {
    vec2 p = n.xy * (1.0 / (abs(n.x) + abs(n.y) + abs(n.z)));
    if (n.z <= 0.0)
        p = (1.0 - abs(p.yx)) * vec2(p.x >= 0.0 ? 1.0 : -1.0, p.y >= 0.0 ? 1.0 : -1.0);
    return p;
}

// Samples one probe's octahedral tile in the irradiance atlas in direction N.
vec3 DDGI_SampleProbe(int probeIndex, vec3 N) {
    int stride = ddgi_tileSize + 2;
    int col = probeIndex % ddgi_atlasProbesPerRow;
    int row = probeIndex / ddgi_atlasProbesPerRow;

    vec2 oct = DDGI_OctEncode(N);
    vec2 texelInTile = (oct * 0.5 + 0.5) * float(ddgi_tileSize);

    vec2 atlasTexel = vec2(col * stride, row * stride) + vec2(1.0) + texelInTile;
    vec2 atlasUV = atlasTexel / float(ddgi_atlasSize);

    return texture(ddgi_irradianceAtlas, atlasUV).rgb;
}

// Same octahedral tile lookup as DDGI_SampleProbe, against the distance atlas instead - returns
// (mean hit distance, mean hit distance^2) for that probe in direction `dir`.
vec2 DDGI_SampleDistance(int probeIndex, vec3 dir) {
    int stride = ddgi_tileSize + 2;
    int col = probeIndex % ddgi_atlasProbesPerRow;
    int row = probeIndex / ddgi_atlasProbesPerRow;

    vec2 oct = DDGI_OctEncode(dir);
    vec2 texelInTile = (oct * 0.5 + 0.5) * float(ddgi_tileSize);

    vec2 atlasTexel = vec2(col * stride, row * stride) + vec2(1.0) + texelInTile;
    vec2 atlasUV = atlasTexel / float(ddgi_atlasSize);

    return texture(ddgi_distanceAtlas, atlasUV).rg;
}

// Chebyshev's inequality applied to a probe's stored (mean, mean^2) hit-distance distribution, to
// estimate how likely that probe can actually "see" a point `distToPoint` away without a wall between
// them - this (not just the trilinear grid weight) is what stops light/shadow from leaking through
// geometry the way a plain irradiance-only probe blend does (e.g. sun hitting a roof lighting the
// ceiling directly below it). Cubing the raw Chebyshev bound sharpens the falloff so partially-occluded
// probes fade out faster than a linear bound would - see the identical helper in probe_trace.comp's
// SampleIndirect for the sibling copy this mirrors (needed separately there for the bounce-feedback
// loop, which reads a different, not-yet-published atlas).
float DDGI_VisibilityWeight(vec2 meanMean2, float distToPoint) {
    float mean = meanMean2.x;
    if (distToPoint <= mean)
        return 1.0;

    float variance = abs(meanMean2.y - mean * mean);
    float d = distToPoint - mean;
    float chebyshev = variance / (variance + d * d);
    return max(chebyshev * chebyshev * chebyshev, 0.0);
}

// Trilinearly blends the 8 probes surrounding worldPos, each sampled toward N, and combines the result
// with albedo/metallic the same way IBL_Diffuse does - a drop-in replacement for it when a probe volume
// is active (see ddgi_enabled in main()). Each probe's trilinear grid weight is further scaled by
// DDGI_VisibilityWeight so an occluded probe (behind a wall from worldPos) contributes little or nothing,
// regardless of how close it is in the grid.
vec3 DDGI_Diffuse(vec3 worldPos, vec3 N, vec3 albedo, float metallic) {
    vec3 gridPos = (worldPos - ddgi_gridOrigin) / max(ddgi_gridSpacing, vec3(1e-4));
    vec3 base = floor(gridPos);
    vec3 frac = clamp(gridPos - base, 0.0, 1.0);

    vec3 irradiance = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 8; i++) {
        vec3 offset = vec3(float(i & 1), float((i >> 1) & 1), float((i >> 2) & 1));
        vec3 probeCoord = clamp(base + offset, vec3(0.0), max(ddgi_probeCounts - 1.0, 0.0));

        vec3 w = mix(1.0 - frac, frac, offset);
        float trilinearWeight = w.x * w.y * w.z;
        if (trilinearWeight <= 0.0)
            continue;

        int probeIndex = int(probeCoord.x)
            + int(probeCoord.y) * int(ddgi_probeCounts.x)
            + int(probeCoord.z) * int(ddgi_probeCounts.x) * int(ddgi_probeCounts.y);

        // Visibility test direction is probe -> shading point, NOT N (the irradiance sample direction
        // below) - a probe can be occluded from a point regardless of that point's surface normal.
        vec3 probeWorldPos = ddgi_gridOrigin + probeCoord * ddgi_gridSpacing;
        vec3 toPoint = worldPos - probeWorldPos;
        float distToPoint = length(toPoint);
        vec3 dirToPoint = toPoint / max(distToPoint, 1e-5);

        float visWeight = DDGI_VisibilityWeight(DDGI_SampleDistance(probeIndex, dirToPoint), distToPoint);
        float weight = trilinearWeight * visWeight;
        if (weight <= 0.0)
            continue;

        irradiance += DDGI_SampleProbe(probeIndex, N) * weight;
        totalWeight += weight;
    }

    // Deliberately no epsilon floor on totalWeight : if every surrounding probe is occluded from this
    // point (e.g. a fully sealed room lit only from outside), the correct result is 0 (no bounce light),
    // not a dim leak propped up by a fallback weight.
    if (totalWeight > 0.0)
        irradiance /= totalWeight;

    vec3 kS = mix(vec3(0.04), albedo, metallic);
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    return irradiance * albedo * kD;
}

vec3 IBL_Specular(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness){
    vec3 R = reflect(-V, N);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Sample BRDF LUT (NdotV, roughness)
    float NdotV = max(dot(N, V), 0.0);
    vec2 brdf = texture(ibl_brdfLUT, vec2(NdotV, roughness)).rg;

    // Sample prefiltered env map using roughness mip level
    const float MAX_REFLECTION_LOD = 5.0;
    vec3 prefilteredColor = textureLod(ibl_prefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD).rgb;

    // Fresnel-Schlick for IBL  
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

    return prefilteredColor * (F * brdf.x + brdf.y);
}

vec3 ComputeLightDisney(Light light, vec3 L, vec3 V, vec3 N, vec3 baseColor, float roughness, float metallic, float visibility, float attenuation) {
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
    vec3 Lo = (diffuse + specular) * NdotL * light.color.xyz * light.intensity * visibility * attenuation;

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

        float visibility = 1.0;
        float attenuation = 1.0;
        vec3 L = vec3(0.0);

        if (l.type == 0) { // Directional
            L = -normalize(l.direction.xyz);
            if (l.castShadow == 1) {
                int cascadeIdx = selectCascade(dirIdx);
                visibility = ShadowCalculationDir(dirShadowMaps[cascadeIdx], dirLightSpaceMatrices[cascadeIdx], L, worldPos, worldNormal);
            }
            
            dirIdx++;
        }
        else if (l.type == 1) { // Point
            vec3 toLight = l.position.xyz - worldPos;
            
            float dist = length(toLight);

            L = normalize(toLight);

            attenuation = 1.0 / (dist * dist);

            float falloff = clamp(1.0 - dist / l.radius, 0.0, 1.0);
            attenuation *= falloff * falloff;

            if (l.castShadow == 1){
                visibility = ShadowCalculationPoint(pointIdx, l.position.xyz, worldPos, pointLightFarPlanes[pointIdx], worldNormal);
            }
            pointIdx++;
        }
        else if (l.type == 2) { // Spot
            vec3 toFrag = worldPos - l.position.xyz;

            float dist = length(toFrag);
            
            L = normalize(l.position.xyz - worldPos);
            
            // distance attenuation
            attenuation = 1.0 / (dist * dist);

            float falloff = clamp(1.0 - dist / l.radius, 0.0, 1.0);
            attenuation *= falloff * falloff;

            // cone attenuation
            float cosTheta = dot(L, normalize(-l.direction.xyz));

            float epsilon = l.innerCutoff - l.outerCutoff;
            float spotIntensity = clamp((cosTheta - l.outerCutoff) / epsilon, 0.0, 1.0);

            attenuation *= spotIntensity;

            if (l.castShadow == 1) {
                visibility = ShadowCalculationSpot(spotShadowMaps[spotIdx], L, spotLightSpaceMatrices[spotIdx], worldNormal);
            }
                
            spotIdx++;
        }

        result += ComputeLightDisney(l, L, V, worldNormal, baseColor.rgb, roughnessValue, metallicValue, visibility, attenuation);
    }

    // Real-time GI (ddgi_enabled) intentionally isn't gated behind useEnvReflections - that flag toggles
    // the (costlier) specular env reflections per material, but diffuse GI from an active probe volume
    // should show up on every material regardless, or it silently does nothing on any material that
    // doesn't happen to have useEnvReflections set.
    vec3 ambientDiffuse;
    vec3 specularIBL = vec3(0.0);

    if (ddgi_enabled) {
        ambientDiffuse = DDGI_Diffuse(worldPos, worldNormal, baseColor.rgb, metallicValue);
        if (useEnvReflections)
            specularIBL = IBL_Specular(worldNormal, V, baseColor.rgb, metallicValue, roughnessValue);
    }
    else if (useEnvReflections) {
        ambientDiffuse = IBL_Diffuse(worldNormal, baseColor.rgb, metallicValue);
        specularIBL = IBL_Specular(worldNormal, V, baseColor.rgb, metallicValue, roughnessValue);
    }
    else {
        ambientDiffuse = baseColor.rgb * ambientIntensity;
    }

    vec3 lighting = result + ambientDiffuse + specularIBL;
    fragColor = vec4(lighting, baseColor.a);
}