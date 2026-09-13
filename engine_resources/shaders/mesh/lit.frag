#version 430 core
// Needed only for ssaoTextureHandle's sampler2D(uvec2) reconstruction below (see its declaration) - the
// blurred AO texture is passed as a bindless handle rather than a `layout(binding=N)` sampler because
// this shader is already close to a real, driver-documented texture-unit limit (see kMaxProbeVolumes in
// probe_manager.hpp).
#extension GL_ARB_bindless_texture : enable

#include "../common/light.glsl"
#include "../common/cluster.glsl"

layout(std430, binding = 0) buffer LightBuffer {
    Light lights[];
};

// Clustered light culling (point/spot lights only - see LightCullingManager). Filled every frame by
// cluster_light_cull.comp; directional lights bypass this entirely (see the dedicated loop in main()).
layout(std430, binding = 2) readonly buffer LightGrid {
    uvec2 clusters[]; // x = offset into lightIndices, y = count
};
layout(std430, binding = 3) readonly buffer LightIndexList {
    uint lightIndices[];
};

uniform uvec2 clusterGridSizeXY;
uniform float clusterScaleZ;
uniform float clusterBiasZ;

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

// Real-time diffuse GI (see ProbeManager) - falls back to the static IBL diffuse term below when no
// probe volume's grid contains the shading point (DDGI_PickVolume returns -1). Up to this many
// ProbeVolumes can be active at once, each with its own grid/atlases; DDGI_PickVolume picks the
// smallest one containing the point, so a dense local volume overrides a coarse room-scale one it's
// nested in. MUST equal ProbeManager::kMaxProbeVolumes - gl_api.cpp fills ddgi_*[0 .. volumeCount-1].
#define MAX_PROBE_VOLUMES 2

// One octahedral irradiance tile atlas + one (mean dist, mean dist^2) distance atlas per volume. These
// are scalar samplers on hand-picked units (7,8,9,19 - the gaps left between the material/IBL block
// and the shadow-map ranges at 10/20/30) rather than sampler2D[MAX_PROBE_VOLUMES] arrays : GLSL
// sampler-array indexing must be dynamically uniform (the volume index here is per-fragment), and a
// 2-element array from binding 7 would also push the distance atlas onto unit 32, past the 32-unit
// limit some drivers enforce (see kMaxProbeVolumes in probe_manager.hpp). The distance atlas feeds
// DDGI_VisibilityWeight, which stops an occluded probe (wall between it and the shading point) from
// leaking light/shadow through the trilinear blend. Add a third volume -> add ddgi_*Atlas2 here, in
// DDGI_Sample*Atlas below, in DDGI_ProbeState, and bump MAX_PROBE_VOLUMES / kMaxProbeVolumes together.
layout(binding = 7)  uniform sampler2D ddgi_irradianceAtlas0;
layout(binding = 8)  uniform sampler2D ddgi_irradianceAtlas1;
layout(binding = 9)  uniform sampler2D ddgi_distanceAtlas0;
layout(binding = 19) uniform sampler2D ddgi_distanceAtlas1;

uniform bool ddgi_enabled;
uniform int  ddgi_volumeCount;                     // ready volumes actually filled in below, <= MAX_PROBE_VOLUMES
uniform vec3 ddgi_gridOrigin[MAX_PROBE_VOLUMES];
uniform vec3 ddgi_gridSpacing[MAX_PROBE_VOLUMES];
uniform vec3 ddgi_probeCounts[MAX_PROBE_VOLUMES];  // ivec3 stored as vec3
uniform int  ddgi_tileSize[MAX_PROBE_VOLUMES];
uniform int  ddgi_atlasProbesPerRow[MAX_PROBE_VOLUMES];
uniform int  ddgi_atlasSize[MAX_PROBE_VOLUMES];

// Per-probe state buffer, one per volume (SSBO 14, 15 - see gl_api.cpp / probe_manager.hpp). GLSL 4.3
// can't put an unsized array inside an arrayed interface block portably, so it's one named block per
// volume plus the DDGI_ProbeState() dispatch below. .w = classification flag (0 = probe embedded in
// geometry, contributes nothing), .xyz = relocation offset added to the probe's grid position.
layout(std430, binding = 14) readonly buffer DDGIProbeState0 { vec4 ddgi_probeState0[]; };
layout(std430, binding = 15) readonly buffer DDGIProbeState1 { vec4 ddgi_probeState1[]; };

vec4 DDGI_ProbeState(int v, int probeIndex) {
    if (v == 0) return ddgi_probeState0[probeIndex];
    return ddgi_probeState1[probeIndex];
}

layout(binding = 10) uniform sampler2DShadow dirShadowMaps[NUM_CASCADES];
layout(binding = 20) uniform sampler2D spotShadowMaps[10];

layout(binding = 30) uniform samplerCubeArray pointShadowMapArray;

// PBR values
const float PI = 3.141592653589793;
uniform float metallic;
uniform float roughness;
uniform float ambientIntensity;

// Screen-space ambient occlusion (see SSAOManager) - ssaoTextureHandle is a bindless texture handle
// (low/high 32 bits of an ARB_bindless_texture handle), reconstructed into a
// sampler only at the point of use (SampleSSAO below) via GLSL's sampler2D(uvec2) constructor - NOT
// declared as `uniform sampler2D`, which would consume one of this shader's already-scarce texture
// units. ssaoIntensity blends toward "no occlusion" in SampleSSAO,
// so it's a cheap post-hoc strength control that doesn't need the SSAO passes themselves rerun.
uniform bool ssaoEnabled;
uniform float ssaoIntensity;
uniform uvec2 ssaoTextureHandle;

vec3 SampleSSAO(vec3 ambient) {
    if (!ssaoEnabled || ssaoTextureHandle == uvec2(0))
        return ambient;

    sampler2D ssaoTex = sampler2D(ssaoTextureHandle);
    float ao = texture(ssaoTex, gl_FragCoord.xy / vec2(textureSize(ssaoTex, 0))).r;
    // ssaoIntensity is allowed above 1.0 as an amplification knob (mix() extrapolates past `ao`
    // once the blend factor exceeds 1) - clamp so an aggressive value can't overshoot into
    // negative/relighting territory.
    return ambient * clamp(mix(1.0, ao, ssaoIntensity), 0.0, 1.0);
}

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

// Flattened index into LightGrid/LightIndexList for the cluster this fragment sits in - must match
// the flattening cluster_build.comp/cluster_light_cull.comp use (x + y*gridWidth + z*gridWidth*gridHeight).
uint GetClusterIndex(vec3 fragWorldPos){
    vec4 viewPosForCluster = viewMatrix * vec4(fragWorldPos, 1.0);
    float viewZ = max(-viewPosForCluster.z, 1e-4);

    uint zSlice = uint(clamp(log2(viewZ) * clusterScaleZ + clusterBiasZ, 0.0, float(CLUSTER_Z_SLICES - 1)));

    uvec2 tileXY = uvec2(gl_FragCoord.xy) / uint(CLUSTER_TILE_PX);
    tileXY = min(tileXY, clusterGridSizeXY - uvec2(1, 1));

    return tileXY.x + tileXY.y * clusterGridSizeXY.x + zSlice * (clusterGridSizeXY.x * clusterGridSizeXY.y);
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

#include "../common/octahedral.glsl"
#include "../common/ddgi_atlas.glsl"
#include "../common/ddgi_visibility.glsl"

// Per-fragment volume select over the scalar atlas samplers (see their declaration for why they're
// not arrays). Hard-codes MAX_PROBE_VOLUMES == 2, same as DDGI_ProbeState above.
// textureLod(.., 0) not texture() : the atlases have no mips, and the select below is non-uniform
// control flow where implicit-LOD derivatives are undefined.
vec3 DDGI_SampleIrradianceAtlas(int v, vec2 uv) {
    return (v == 0) ? textureLod(ddgi_irradianceAtlas0, uv, 0.0).rgb
                    : textureLod(ddgi_irradianceAtlas1, uv, 0.0).rgb;
}
vec2 DDGI_SampleDistanceAtlas(int v, vec2 uv) {
    return (v == 0) ? textureLod(ddgi_distanceAtlas0, uv, 0.0).rg
                    : textureLod(ddgi_distanceAtlas1, uv, 0.0).rg;
}

// Samples one probe's octahedral tile in volume v's irradiance atlas in direction N - see
// common/ddgi_atlas.glsl's DDGI_AtlasUV for the shared tile-addressing math this builds on.
vec3 DDGI_SampleProbe(int v, int probeIndex, vec3 N) {
    vec2 uv = DDGI_AtlasUV(probeIndex, N, ddgi_tileSize[v], ddgi_atlasProbesPerRow[v], ddgi_atlasSize[v]);
    return DDGI_SampleIrradianceAtlas(v, uv);
}

// Same tile lookup against the distance atlas - returns (mean hit distance, mean hit distance^2).
vec2 DDGI_SampleDistance(int v, int probeIndex, vec3 dir) {
    vec2 uv = DDGI_AtlasUV(probeIndex, dir, ddgi_tileSize[v], ddgi_atlasProbesPerRow[v], ddgi_atlasSize[v]);
    return DDGI_SampleDistanceAtlas(v, uv);
}

// Index of the smallest active volume whose grid AABB contains p, or -1 if none does. "Smallest" =
// smallest cell volume, so a dense local ProbeVolume nested inside a coarse room-scale one wins for
// points it covers, with no explicit priority field (see ProbeVolume's header comment).
int DDGI_PickVolume(vec3 p) {
    int best = -1;
    float bestCell = 1e30;
    for (int v = 0; v < ddgi_volumeCount; v++) {
        vec3 gmin = ddgi_gridOrigin[v];
        vec3 gmax = gmin + ddgi_gridSpacing[v] * max(ddgi_probeCounts[v] - 1.0, vec3(0.0));
        if (all(greaterThanEqual(p, gmin)) && all(lessThanEqual(p, gmax))) {
            float cell = ddgi_gridSpacing[v].x * ddgi_gridSpacing[v].y * ddgi_gridSpacing[v].z;
            if (cell < bestCell) { bestCell = cell; best = v; }
        }
    }
    return best;
}

// Trilinearly blends the 8 probes of volume `v` surrounding worldPos, each sampled toward N, and
// combines the result with albedo/metallic the same way IBL_Diffuse does. Each probe's trilinear grid
// weight is further scaled by DDGI_VisibilityWeight (occluded probe -> ~0) and by the probe's
// classification flag (probe embedded in geometry -> 0), and every probe position includes its
// relocation offset (see DDGI_ProbeState / probe_relocate.comp).
vec3 DDGI_Diffuse(int v, vec3 worldPos, vec3 N, vec3 albedo, float metallic) {
    // Bias the sampled position off the surface along its normal before gridding, same as
    // probe_trace.comp's SampleIndirect (see that function's comment) - kept in sync since they're
    // sibling copies of the same trilinear-probe-blend logic (one feeding the next bounce, this one
    // shading what the player actually sees). Missing this bias here used to mean the *final* shaded
    // pixel sampled the grid right at the surface it sits on, which is far more prone to picking up the
    // wrong side of a nearby occlusion boundary than the (correctly biased) bounce-feedback path was -
    // most visible as weak/missing colored bounce light right where two differently-tinted surfaces
    // meet (e.g. a colored curtain near the floor).
    vec3 biasedPos = worldPos + N * (0.25 * min(min(ddgi_gridSpacing[v].x, ddgi_gridSpacing[v].y), ddgi_gridSpacing[v].z));

    vec3 gridPos = (biasedPos - ddgi_gridOrigin[v]) / max(ddgi_gridSpacing[v], vec3(1e-4));
    vec3 base = floor(gridPos);
    vec3 frac = clamp(gridPos - base, 0.0, 1.0);

    vec3 irradiance = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 8; i++) {
        vec3 offset = vec3(float(i & 1), float((i >> 1) & 1), float((i >> 2) & 1));
        vec3 probeCoord = clamp(base + offset, vec3(0.0), max(ddgi_probeCounts[v] - 1.0, 0.0));

        vec3 w = mix(1.0 - frac, frac, offset);
        float trilinearWeight = w.x * w.y * w.z;
        if (trilinearWeight <= 0.0)
            continue;

        int probeIndex = int(probeCoord.x)
            + int(probeCoord.y) * int(ddgi_probeCounts[v].x)
            + int(probeCoord.z) * int(ddgi_probeCounts[v].x) * int(ddgi_probeCounts[v].y);

        vec4 state = DDGI_ProbeState(v, probeIndex);
        if (state.w <= 0.0) // probe classified as embedded in geometry
            continue;

        // Visibility test direction is probe -> (biased) shading point, NOT N (the irradiance sample
        // direction below) - a probe can be occluded from a point regardless of that point's surface
        // normal. + state.xyz : the probe may have been relocated out of geometry.
        vec3 probeWorldPos = ddgi_gridOrigin[v] + probeCoord * ddgi_gridSpacing[v] + state.xyz;
        vec3 toPoint = biasedPos - probeWorldPos;
        float distToPoint = length(toPoint);
        vec3 dirToPoint = toPoint / max(distToPoint, 1e-5);

        float visWeight = DDGI_VisibilityWeight(DDGI_SampleDistance(v, probeIndex, dirToPoint), distToPoint);
        float weight = trilinearWeight * visWeight;
        if (weight <= 0.0)
            continue;

        irradiance += DDGI_SampleProbe(v, probeIndex, N) * weight;
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

    // Directional lights: never clustered (at most a handful in any scene - see
    // LightManager::MAX_DIRECTIONAL_LIGHTS), so still a plain scan of the full light list.
    for (int i = 0; i < lightNB; ++i) {
        Light l = lights[i];
        if (l.type != 0)
            continue;

        vec3 L = -normalize(l.direction.xyz);
        float visibility = 1.0;

        if (l.castShadow == 1 && l.shadowIndex >= 0) {
            int cascadeIdx = selectCascade(l.shadowIndex);
            visibility = ShadowCalculationDir(dirShadowMaps[cascadeIdx], dirLightSpaceMatrices[cascadeIdx], L, worldPos, worldNormal);
        }

        result += ComputeLightDisney(l, L, V, worldNormal, baseColor.rgb, roughnessValue, metallicValue, visibility, 1.0);
    }

    // Point/Spot lights: only the subset the light-culling compute pass placed in this fragment's
    // cluster (see LightCullingManager / cluster_light_cull.comp) - not the full light list.
    uint clusterIndex = GetClusterIndex(worldPos);
    uvec2 clusterEntry = clusters[clusterIndex];
    uint clusterLightOffset = clusterEntry.x;
    uint clusterLightCount = min(clusterEntry.y, uint(MAX_LIGHTS_PER_CLUSTER));

    for (uint k = 0; k < clusterLightCount; ++k) {
        Light l = lights[lightIndices[clusterLightOffset + k]];

        float visibility = 1.0;
        float attenuation = 1.0;
        vec3 L = vec3(0.0);

        if (l.type == 1) { // Point
            vec3 toLight = l.position.xyz - worldPos;

            float dist = length(toLight);

            L = normalize(toLight);

            attenuation = 1.0 / (dist * dist);

            float falloff = clamp(1.0 - dist / l.radius, 0.0, 1.0);
            attenuation *= falloff * falloff;

            if (l.castShadow == 1 && l.shadowIndex >= 0){
                visibility = ShadowCalculationPoint(l.shadowIndex, l.position.xyz, worldPos, pointLightFarPlanes[l.shadowIndex], worldNormal);
            }
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

            if (l.castShadow == 1 && l.shadowIndex >= 0) {
                visibility = ShadowCalculationSpot(spotShadowMaps[l.shadowIndex], L, spotLightSpaceMatrices[l.shadowIndex], worldNormal);
            }
        }

        result += ComputeLightDisney(l, L, V, worldNormal, baseColor.rgb, roughnessValue, metallicValue, visibility, attenuation);
    }

    // Real-time GI (ddgi_enabled) intentionally isn't gated behind useEnvReflections - that flag toggles
    // the (costlier) specular env reflections per material, but diffuse GI from an active probe volume
    // should show up on every material regardless, or it silently does nothing on any material that
    // doesn't happen to have useEnvReflections set. A point outside every active volume's grid
    // (ddgiVolume < 0) falls back to the same IBL / flat-ambient path as when no volume exists.
    vec3 ambientDiffuse = vec3(0.0);
    vec3 specularIBL = vec3(0.0);

    int ddgiVolume = ddgi_enabled ? DDGI_PickVolume(worldPos) : -1;

    if (ddgiVolume >= 0) {
        ambientDiffuse = DDGI_Diffuse(ddgiVolume, worldPos, worldNormal, baseColor.rgb, metallicValue);
        if (useEnvReflections)
            specularIBL = IBL_Specular(worldNormal, V, baseColor.rgb, metallicValue, roughnessValue);
    }
    else if (useEnvReflections) {
        ambientDiffuse = IBL_Diffuse(worldNormal, baseColor.rgb, metallicValue);
        specularIBL = IBL_Specular(worldNormal, V, baseColor.rgb, metallicValue, roughnessValue);
    }

    // Flat ambient floor, ADDED on top of whatever DDGI/IBL/direct lighting already computed above
    // (not multiplied into it, and not gated behind "no DDGI and no IBL" the way this used to be) : this
    // is what makes ambientIntensity mean "light that's there even when the scene has no lights at all"
    // - a real light source can't do that (zero lights = zero direct contribution, always), only a flat
    // additive term can. Being additive rather than a multiplier also means it still does something in
    // a spot DDGI/IBL computes near-zero for (a probe-starved corner, geometry outside every volume's
    // grid), instead of just scaling an already-dark result to a still-dark one. Level::ambientIntensity
    // defaults to 0.0 - DDGI/IBL are real, computed lighting and don't need a fake floor propping them
    // up by default, so this stays a no-op unless a level explicitly wants unlit corners not to be pure
    // black, or a material overrides it for a cheap self-lit look (light_bulb.mat : 4.0, useEnvReflections
    // = false, so this term is the ENTIRE reason it's visibly bright at all - see GLRendererAPI::
    // BindLevelState for how a level default vs. a per-material override of this uniform interact).
    ambientDiffuse += baseColor.rgb * ambientIntensity;

    // Screen-space ambient occlusion (see SampleSSAO above / SSAOManager) - darkens ambient/indirect
    // light in creases/corners/contact points, not `result` (direct lighting) or `specularIBL` : that's
    // the standard, well-understood scope for SSAO, and it composes naturally with the additive floor
    // just above (a level with SSAO on but zero real lights/DDGI/IBL still gets its ambientIntensity
    // floor darkened in contact points, which is correct - occlusion blocks that flat "always there"
    // light too, physically). The 3 SSAO passes always run (see SSAOManager::Init) - ssaoEnabled is
    // purely a lit.frag-side gate, off is a no-op.
    ambientDiffuse = SampleSSAO(ambientDiffuse);

    vec3 lighting = result + ambientDiffuse + specularIBL;
    fragColor = vec4(lighting, baseColor.a);
}