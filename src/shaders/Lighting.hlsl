#include "common/BRDF.hlsli"
#include "common/LightStruct.hlsli"

Texture2D gbuffer_albedo : register(t0);
Texture2D gbuffer_normal : register(t1);
Texture2D gbuffer_roughness : register(t2);
Texture2D gbuffer_depth : register(t3);
Texture2D shadow_mask : register(t4);
TextureCube irradiance_map : register(t5);
TextureCube prefiltered_env_map : register(t6);
Texture2D smooth_normals : register(t7);
Texture2D brdf_LUT_env : register(t8);


SamplerState texSampler : register(s0);

static const int numSamplesJimenezSep = 15;
static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f / 2.2f;
//static const float SCREEN_WIDTH = 1280.0f;
//static const float SCREEN_HEIGHT = 720.0f;
static const float SCREEN_WIDTH = 1920.0f;
static const float SCREEN_HEIGHT = 1080.0f;

#define DIRECTIONAL_LIGHT_ID 0
#define POINT_LIGHT_ID 1
#define SPOT_LIGHT_ID 2
#define INDIRECT_LIGHT_ID 3

struct PS_OUT
{
    float4 scene_lighted : SV_TARGET0;
    float4 diffuse : SV_TARGET1;
    float4 specular : SV_TARGET2;
    float4 transmitted : SV_TARGET3;
    float4 ambient : SV_TARGET4;
};

struct ExitRadiance_t
{
    float3 diffuse;
    float3 specular;
    float3 transmitted;
};

struct ModelSSS
{
    //golubev subsurface scattering model
    float4 samples[16];
    float4 subsurfaceAlbedo;
    float4 meanFreePathColor;
    
    float meanFreePathDist;
    float scale;
    
    float2 dir;
    int numSamples;
    int3 pad2;

    //golubev subsurface scattering model
    float4 kernel[numSamplesJimenezSep];
    //float4 kernel[16];
};

cbuffer light : register(b0)
{
    Directional_Light dirLight[1];
    
    int numPLights;
    int numSpotLights;
    int2 pad;
    
    Point_Light Point_Lights[16];
    float4 posPoint_LightViewSpace[16];
    
    Spot_Light spotLights[16];
    float4 posSpotLightViewSpace[16];
    float4 dirSpotLightViewSpace[16];
    
    ModelSSS sssModels[64];
}

cbuffer omniDirShadowMatrixes : register(b2)
{
    float4x4 dirLightViewMat[1];
    float4x4 dirLightProjMat[1];
    
    float4x4 lightViewMat[16 * 6];
    float4x4 lightProjMat;
    
    float4x4 spotLightViewMat[16];
    float4x4 spotLightProjMat[16];
}

cbuffer draw_constants : register(b3)
{
    uint light_index;
    uint light_type;
}

cbuffer camMat : register(b5)
{
    float4x4 invCamProjMat;
    float4x4 invCamViewMat;
}

ExitRadiance_t computeColDirLight(float3 base_color, Directional_Light dLight, float3 viewDir, float3 normal, float3 smoothNormal, float metalness, float roughness, float thickness, float4 posWS, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat, uint pixel_sss_model_id);
ExitRadiance_t computeColPoint_Light(float3 base_color, Point_Light pLight, float4 posLightViewSpace, float4 fragPosViewSpace, float3 normal, float3 viewDir, float metalness, float roughness, float transmDist, uint pixel_sss_model_id);
ExitRadiance_t computeColIndirect_Light(float3 base_color, float4 fragPosViewSpace, float3 normal, float3 viewDir, float metalness, float roughness);
ExitRadiance_t computeColSpotLight(float3 base_color, int lightIndex, float3 viewDir, float3 normal, float metalness, float roughness, float4 posWS, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat, uint pixel_sss_model_id);

float3 T(float s);
float3 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat);
float computeTransmDist(float4 positionWS, float nearPlane, float farPlane, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat, float correctedBias, Texture2D shadowMap, float3 normalVS);

float3 decode_normal(float2 f)
{
    f = f * 2.0 - 1.0;
 
    float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = saturate(-n.z);
    n.xy += n.xy >= 0.0 ? -t : t;
    
    return normalize(n);
}

PS_OUT ps(float4 fragPos : SV_POSITION)
{
    float2 uvCoord = float2(fragPos.x / SCREEN_WIDTH, fragPos.y / SCREEN_HEIGHT);
    
    float sampledDepth = gbuffer_depth.Sample(texSampler, uvCoord).r;
    float4 albedo_sample = gbuffer_albedo.Sample(texSampler, uvCoord).rgba;
    //float3 albedo = pow(albedo_sample.rgb, (1.0f / gamma));
    float3 normal = decode_normal(gbuffer_normal.Sample(texSampler, uvCoord).rg);
    //float3 normal = gbuffer_normal.Sample(texSampler, uvCoord).xyz * 2.0f - 1.0f;
    float3 smoothNormal = smooth_normals.Sample(texSampler, uvCoord).xyz * 2.0f - 1.0f;
    float3 roughMetThick = gbuffer_roughness.Sample(texSampler, uvCoord).rgb;
    float shadowing_factor = shadow_mask.Sample(texSampler, uvCoord).r;
    
    float linRoughness = pow(roughMetThick.r, 4.0f);
    float metalness = roughMetThick.g;
    float thickness = roughMetThick.b;
    
    uint sssParamsPack = albedo_sample.a * 255.0f;
    uint pixel_sss_model_id = (sssParamsPack & 3);
    
    float3 positionVS = posFromDepth(uvCoord, sampledDepth, invCamProjMat);
    float4 positionWS = mul(float4(positionVS, 1.0f), invCamViewMat);
    
    float3 viewDir = -normalize(positionVS).xyz;
    viewDir = mul(float4(viewDir, 0.0f), invCamViewMat);
    
    ExitRadiance_t radExitance;
    radExitance.diffuse = float3(0.0f, 0.0f, 0.0f);
    radExitance.specular = float3(0.0f, 0.0f, 0.0f);
    radExitance.transmitted = float3(0.0f, 0.0f, 0.0f);
    
    switch(light_type)
    {
        case DIRECTIONAL_LIGHT_ID:{
                ExitRadiance_t tmpRadExitance = computeColDirLight(albedo_sample.rgb, dirLight[light_index], viewDir, normal, smoothNormal, metalness, linRoughness, thickness, positionWS, dirLightViewMat[light_index], dirLightProjMat[light_index], pixel_sss_model_id);

                radExitance.diffuse = tmpRadExitance.diffuse;
                radExitance.specular = tmpRadExitance.specular;
                radExitance.transmitted = tmpRadExitance.transmitted;
                break;
            }
        case SPOT_LIGHT_ID:{
                ExitRadiance_t tmpRadExitance = computeColSpotLight(albedo_sample.rgb, light_index, viewDir, normal, metalness, linRoughness, positionWS, spotLightViewMat[light_index], spotLightProjMat[light_index], pixel_sss_model_id);
        
                radExitance.diffuse = tmpRadExitance.diffuse;
                radExitance.specular = tmpRadExitance.specular;
                radExitance.transmitted = tmpRadExitance.transmitted;
                break;
            }
        case POINT_LIGHT_ID:{
                float transmDist = 10.0f;
                ExitRadiance_t tmpRadExitance = computeColPoint_Light(albedo_sample.rgb, Point_Lights[light_index], posPoint_LightViewSpace[light_index], positionWS, normal, viewDir, metalness, linRoughness, transmDist, pixel_sss_model_id);
        
                radExitance.diffuse = tmpRadExitance.diffuse;
                radExitance.specular = tmpRadExitance.specular;
                radExitance.transmitted = tmpRadExitance.transmitted;
                break;
            }
        case INDIRECT_LIGHT_ID:{
                ExitRadiance_t tmpRadExitance = computeColIndirect_Light(albedo_sample.rgb, positionWS, normal, viewDir, metalness, linRoughness);
            
                radExitance.diffuse = tmpRadExitance.diffuse;
                radExitance.specular = tmpRadExitance.specular;
                radExitance.transmitted = tmpRadExitance.transmitted;
            
                shadowing_factor = 1.0f;
                break;
            }
    }

    PS_OUT ps_out;
    ps_out.diffuse = shadowing_factor * float4(radExitance.diffuse, 1.0f);
    ps_out.specular = shadowing_factor * float4(radExitance.specular, 1.0f);
    ps_out.transmitted = float4(radExitance.transmitted, 1.0f);
    
    ps_out.ambient = float4(0.00f, 0.0f, 0.0f, 1.0f); /* * max(0.0f, dot(normal.xyz, dirLight.dir.xyz))*/
    ps_out.scene_lighted = shadowing_factor * (ps_out.diffuse + ps_out.specular);
    
    return ps_out;
}

ExitRadiance_t computeColDirLight(float3 base_color, Directional_Light dLight, float3 viewDir, float3 normal, float3 smoothNormal, float metalness, float linRoughness, float thickness, float4 posWS, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat, uint pixel_sss_model_id)
{
    ExitRadiance_t exitRadiance;
    float3 lDir = -normalize(dLight.dir.xyz);
    //float3 lRef = reflect(lDir, normal);
    
    float3 halfVec = normalize(lDir + viewDir);

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    float3 transmittedRad = float3(0, 0, 0);
    
    if (pixel_sss_model_id > 0)
    {
        float correctedBias = max(0.002f, 0.005f * (1.0f - abs(dot(lDir, smoothNormal))));
        float transmDist = 0.0f; //computeTransmDist(posWS, dLight.near_plane_dist, dLight.far_plane_dist, lightViewMat, lightProjMat, correctedBias, ShadowMap, smoothNormal);
        float E = max(0.3f + dot(-smoothNormal, -dLight.dir.xyz), 0.0f);
        ////TODO: instead of hardcoded scale, use model scale
        //thickness = clamp(thickness, 0.0f, 0.5f) * 2.0f;
        //float3 transmittedRad = thickness * T(transmDist * 100.0f) * E;
    
        //float E = max(0.2f + dot(-normal, lDir), 0.0f);
        transmittedRad = /*thickness * */T(transmDist * 50.0f) * E;
    }

    float3 reflectance = float3(0.04f, 0.04f, 0.04f);
    float3 f0 = lerp(reflectance, base_color, metalness);
    float3 F = F_Schlick(f0, 1.0f, LdotH);
    
    float3 ks = F;
    float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;    
    
    brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness);
    exitRadiance.diffuse = kd * base_color * brdfOut.diffuse * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * (1.0f - metalness);
    exitRadiance.specular = brdfOut.specular * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi;
    exitRadiance.transmitted = transmittedRad * float3(3.14f, 3.14f, 3.14f) * revPi;
    
    return exitRadiance;
}

ExitRadiance_t computeColIndirect_Light(float3 base_color, float4 fragPosViewSpace, float3 normal, float3 viewDir, float metalness, float roughness)
{
    ExitRadiance_t exitRadiance;
    float3 lDir = normal; //-normalize(dLight.dir.xyz);
    //float3 lRef = reflect(lDir, normal);
    
    float3 halfVec = normalize(lDir + viewDir);

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    float3 transmittedRad = float3(0, 0, 0);
    float3 irradiance = irradiance_map.Sample(texSampler, lDir).rgb;

    float3 reflectance = float3(0.04f, 0.04f, 0.04f);
    float3 f0 = lerp(reflectance, base_color, metalness);
    float3 F = F_Schlick(f0, 1.0f, LdotH);
    
    float3 ks = F;
    float3 kd = float3(1.0f,1.0f,1.0f) - ks;
    
    brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, roughness, metalness);
    exitRadiance.diffuse = kd * base_color * brdfOut.diffuse * irradiance * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * (1.0f - metalness);
    
    float3 R = reflect(-viewDir, normal);

    const float MAX_REFLECTION_LOD = 6.0;
    float3 prefilteredColor = prefiltered_env_map.SampleLevel(texSampler, R, MAX_REFLECTION_LOD * roughness).rgb;
    float2 env_brdf = brdf_LUT_env.Sample(texSampler, float2(VdotN, roughness)).rg;

    exitRadiance.specular = prefilteredColor * (F * env_brdf.x + env_brdf.y);
    
    exitRadiance.transmitted = transmittedRad * float3(3.14f, 3.14f, 3.14f) * revPi;
    
    return exitRadiance;
}

ExitRadiance_t computeColPoint_Light(float3 base_color, Point_Light pLight, float4 lightPosViewSpace, float4 fragPosViewSpace, float3 normal, float3 viewDir, float metalness, float linRoughness, float transmDist, uint pixel_sss_model_id)
{
    float3 lDir = pLight.pos_world_space - fragPosViewSpace;
    //float3 lDir = lightPosViewSpace - fragPosViewSpace;
    float dist = length(lDir);
    lDir = normalize(lDir);
    float3 halfVec = normalize(-normalize(fragPosViewSpace.xyz) + lDir.xyz);

    float attenuation = 1 / (pLight.constant + (pLight.lin * dist) + (pLight.quadratic * pow(dist, 2)));

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    float3 transmittedRad = float3(0, 0, 0);
    
    if (pixel_sss_model_id > 0)
    {
        float E = max(0.3f + dot(-normal, lDir), 0.0f);
        transmittedRad = T(transmDist * 100.0f) * E;
    }
    
    float3 reflectance = float3(0.04f, 0.04f, 0.04f);
    float3 f0 = lerp(reflectance, base_color, metalness);
    float3 F = F_Schlick(f0, 1.0f, LdotH);
    
    float3 ks = F;
    float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;
    
    brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness);
       
    ExitRadiance_t exitRadiance;
    exitRadiance.diffuse = kd * base_color * brdfOut.diffuse * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation * (1.0f - metalness);
    exitRadiance.specular = ks * brdfOut.specular * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
    exitRadiance.transmitted = transmittedRad * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
    
    return exitRadiance;
}

ExitRadiance_t computeColSpotLight(float3 base_color, int lightIndex, float3 viewDir, float3 normal, float metalness, float linRoughness, float4 posWS, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat, uint pixel_sss_model_id)
{
    ExitRadiance_t exitRadiance;
    exitRadiance.diffuse = float3(0.0f, 0.0f, 0.0f);
    exitRadiance.specular = float3(0.0f, 0.0f, 0.0f);
    exitRadiance.transmitted = float3(0.0f, 0.0f, 0.0f);

    
    float3 lightDir = normalize(spotLights[lightIndex].pos_world_space - posWS).xyz;
    //float3 lightDir = normalize(posSpotLightViewSpace[lightIndex] - posFragViewSpace);
    float cosAngle = dot(-lightDir, normalize(spotLights[lightIndex].direction.xyz));
    //float cosAngle = dot(-lightDir, normalize(dirSpotLightViewSpace[lightIndex].xyz));
    if (cosAngle > spotLights[lightIndex].cos_cutoff_angle)
    {
        //float3 viewDir = -normalize(posFragViewSpace).xyz;
        float3 halfVec = normalize(viewDir + lightDir);
            
        float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
        float LdotN = saturate(dot(lightDir, normal));
        float LdotH = saturate(dot(lightDir, halfVec));
        float NdotH = saturate(dot(halfVec, normal));
        
        //TODO: provide falloff for spotlight in constant buffer
        float attenuation = cosAngle - spotLights[lightIndex].cos_cutoff_angle;
        
        float3 transmittedRad = float3(0, 0, 0);
        
        if (pixel_sss_model_id > 0)
        {
            float correctedBias = max(0.002f, 0.005f * (1.0f - abs(dot(lightDir, normal))));
            float transmDist = 0.0f; //computeTransmDist(posWS, spotLights[lightIndex].near_plane_dist, spotLights[lightIndex].far_plane_dist, lightViewMat, lightProjMat, correctedBias, SpotShadowMap, normal);
            float E = max(0.3f + dot(-normal, lightDir.xyz), 0.0f);
            transmittedRad = T(transmDist * 100.0f) * E;
        }
        
        float3 reflectance = float3(0.04f, 0.04f, 0.04f);
        float3 f0 = lerp(reflectance, base_color, metalness);
        float3 F = F_Schlick(f0, 1.0f, LdotH);
    
        float3 ks = F;
        float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;
        
        brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness);
        exitRadiance.diffuse = kd * base_color * brdfOut.diffuse * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation * (1.0f - metalness);
        exitRadiance.specular = ks * brdfOut.specular * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
        exitRadiance.transmitted = transmittedRad * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
    }
    
    return exitRadiance;
}

float3 T(float s)
{
    return
    float3(0.233, 0.455, 0.649) * exp(-s * s / 0.0064) +
    float3(0.1, 0.336, 0.344) * exp(-s * s / 0.0484) +
    float3(0.118, 0.198, 0.0) * exp(-s * s / 0.187) +
    float3(0.113, 0.007, 0.007) * exp(-s * s / 0.567) +
    float3(0.358, 0.004, 0.0) * exp(-s * s / 1.99) +
    float3(0.078, 0.0, 0.0) * exp(-s * s / 7.41);
}

float3 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat)
{
    float x = quadTexCoord.x * 2.0f - 1.0f;
    float y = (1.0f - quadTexCoord.y) * 2.0f - 1.0f;
    
    float4 vProjPos = float4(x, y, sampledDepth, 1.0f);
    float4 vPosVS = mul(vProjPos, invMat);
    
    return vPosVS.xyz / vPosVS.w;
}


float computeTransmDist(float4 positionWS, float nearPlane, float farPlane, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat, float correctedBias, Texture2D shadowMap, float3 normalVS)
{
    float4 normalWS = mul(float4(normalVS,0.0f), invCamViewMat);
    float4 shrinkedPos = float4(positionWS.xyz - 0.005f * normalWS.xyz, 1.0f);
    float4 positionLPS = mul(shrinkedPos, lightViewMat);
    float linCurrDepth = positionLPS.z;
    positionLPS = mul(positionLPS, lightProjMat);
    
    float3 ndCoords = positionLPS.xyz / positionLPS.w;
    //float currentDepth = ndCoords.z; /*positionLPS.z; */
    float2 shadowMapUV = (ndCoords.xy + 1.0f) * rcp(2.0f);
    shadowMapUV = float2(shadowMapUV.x, 1.0f - shadowMapUV.y);
    
    float sampDepth = shadowMap.Sample(texSampler, shadowMapUV).r;
    float linSamplDepth = (1.0f - sampDepth) * (farPlane-nearPlane) + nearPlane;
    
    return abs(linSamplDepth - linCurrDepth);
}