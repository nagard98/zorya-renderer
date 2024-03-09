#include "BRDF.hlsl"
#include "LightStruct.hlsl"

Texture2D gbuffer_albedo : register(t0);
Texture2D gbuffer_normal : register(t1);
Texture2D gbuffer_roughness : register(t2);
Texture2D gbuffer_depth : register(t3);
Texture2D smooth_normals : register(t7);

Texture2D ShadowMap : register(t4);
Texture2DArray ShadowCubeMap : register(t5);
//TextureCube ShadowCubeMap : register(t5);
Texture2D SpotShadowMap : register(t6);

SamplerState texSampler : register(s0);

static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f / 2.2f;
static const float SCREEN_WIDTH = 1280.0f;
static const float SCREEN_HEIGHT = 720.0f;

struct PS_OUT
{
    float4 diffuse : SV_TARGET0;
    float4 specular : SV_TARGET1;
    float4 transmitted : SV_TARGET2;
    float4 ambient : SV_TARGET3;
};

struct ExitRadiance_t
{
    float3 diffuse;
    float3 specular;
    float3 transmitted;
};


cbuffer light : register(b0)
{
    DirectionalLight dirLight;
    
    int numPLights;
    int numSpotLights;
    int2 pad;
    
    PointLight pointLights[16];
    float4 posPointLightViewSpace[16];
    
    SpotLight spotLights[16];
    float4 posSpotLightViewSpace[16];
    float4 dirSpotLightViewSpace[16];
};

cbuffer omniDirShadowMatrixes : register(b2)
{
    float4x4 dirLightViewMat;
    float4x4 dirLightProjMat;
    
    float4x4 lightViewMat[16 * 6];
    float4x4 lightProjMat;
    
    float4x4 spotLightViewMat[16];
    float4x4 spotLightProjMat[16];
}

cbuffer camMat : register(b5)
{
    float4x4 invCamProjMat;
    float4x4 invCamViewMat;
}

ExitRadiance_t computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float3 smoothNormal, float metalness, float roughness, float thickness, float4 posWS, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat);
ExitRadiance_t computeColPointLight(PointLight pLight, float4 posLightViewSpace, float4 fragPosViewSpace, float3 normal, float3 viewDir, float metalness, float roughness, float transmDist);
ExitRadiance_t computeColSpotLight(int lightIndex, float4 posFragViewSpace, float3 normal, float metalness, float roughness, float4 posWS, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat);

float3 T(float s);
float3 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat);
float computeTransmDist(float4 positionWS, float nearPlane, float farPlane, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat, float correctedBias, Texture2D shadowMap, float3 normalVS);


PS_OUT ps(float4 fragPos : SV_POSITION)
{
    float2 uvCoord = float2(fragPos.x / SCREEN_WIDTH, fragPos.y / SCREEN_HEIGHT);
    
    float sampledDepth = gbuffer_depth.Sample(texSampler, uvCoord).r;
    float3 albedo = float3(1.0f, 1.0f, 1.0f); //use white as base albedo //pow(gbuffer_albedo.Sample(texSampler, uvCoord).rgb, (1.0f / gamma));
    float3 normal = gbuffer_normal.Sample(texSampler, uvCoord).xyz * 2.0f - 1.0f;
    float3 smoothNormal = smooth_normals.Sample(texSampler, uvCoord).xyz * 2.0f - 1.0f;
    float3 roughMetThick = gbuffer_roughness.Sample(texSampler, uvCoord).rgb;
    float linRoughness = pow(roughMetThick.r, 4.0f);
    float metalness = roughMetThick.g;
    float thickness = roughMetThick.b;
    
    float3 positionVS = posFromDepth(uvCoord, sampledDepth, invCamProjMat);
    float4 positionWS = mul(float4(positionVS, 1.0f), invCamViewMat);
    
    float3 viewDir = -normalize(positionVS).xyz;

    ExitRadiance_t radExitance;
    radExitance.diffuse = float3(0.0f, 0.0f, 0.0f);
    radExitance.specular = float3(0.0f, 0.0f, 0.0f);
    radExitance.transmitted = float3(0.0f, 0.0f, 0.0f);
    
    if (sampledDepth <= 0.9999f)
    {
        if (dirLight.dir.w == 0.0f)
        {
            ExitRadiance_t tmpRadExitance = computeColDirLight(dirLight, viewDir, normal, smoothNormal, metalness, linRoughness, thickness, positionWS, dirLightViewMat, dirLightProjMat);

            radExitance.diffuse += tmpRadExitance.diffuse;
            radExitance.specular += tmpRadExitance.specular;
            radExitance.transmitted += tmpRadExitance.transmitted;
        }
    
        for (int i = 0; i < numSpotLights; i++)
        {
            ExitRadiance_t tmpRadExitance = computeColSpotLight(i, float4(positionVS, 1.0f), normal, metalness, linRoughness, positionWS, spotLightViewMat[i], spotLightProjMat[i]);
        
            radExitance.diffuse += tmpRadExitance.diffuse;
            radExitance.specular += tmpRadExitance.specular;
            radExitance.transmitted += tmpRadExitance.transmitted;
        }
    
        for (int i = 0; i < numPLights; i++)
        {
            float transmDist = 10.0f;
            ExitRadiance_t tmpRadExitance = computeColPointLight(pointLights[i], posPointLightViewSpace[i], float4(positionVS, 1.0f), normal, viewDir, metalness, linRoughness, transmDist);
        
            radExitance.diffuse += tmpRadExitance.diffuse;
            radExitance.specular += tmpRadExitance.specular;
            radExitance.transmitted += tmpRadExitance.transmitted;
        }
    
        radExitance.diffuse = saturate(radExitance.diffuse * albedo/* + 0.03f * albedo*/);
        radExitance.transmitted = radExitance.transmitted * albedo;
    }

    PS_OUT ps_out;
    ps_out.diffuse = float4(pow(radExitance.diffuse, gamma), 1.0f);
    ps_out.specular = float4(pow(radExitance.specular, gamma), 1.0f);
    ps_out.transmitted = float4(pow(radExitance.transmitted, gamma), 1.0f);
    
    ps_out.ambient = float4(pow((albedo * 0.0f/*0.03f*/), gamma), 1.0f);
    return ps_out;
}

ExitRadiance_t computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float3 smoothNormal, float metalness, float linRoughness, float thickness, float4 posWS, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat)
{
    ExitRadiance_t exitRadiance;
    float3 lDir = -normalize(dLight.dir.xyz);
    //float3 lRef = reflect(lDir, normal);
    
    float3 halfVec = normalize(lDir + viewDir);

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    float correctedBias = max(0.002f, 0.005f * (1.0f - abs(dot(lDir, smoothNormal))));
    float transmDist = computeTransmDist(posWS, dLight.nearPlaneDist, dLight.farPlaneDist, lightViewMat, lightProjMat, correctedBias, ShadowMap, smoothNormal);
    float E = max(0.2f + dot(-smoothNormal, -dLight.dir.xyz), 0.0f);
    //TODO: instead of hardcoded scale, use model scale
    thickness = clamp(thickness, 0.0f, 0.5f) * 2.0f;
    float3 transmittedRad = thickness * T(transmDist * 100.0f) * E;
    
    brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness);
    exitRadiance.diffuse = brdfOut.diffuse * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi;
    exitRadiance.specular = brdfOut.specular * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi;
    exitRadiance.transmitted = transmittedRad * float3(3.14f, 3.14f, 3.14f) * revPi;
    
    return exitRadiance;
}

ExitRadiance_t computeColPointLight(PointLight pLight, float4 lightPosViewSpace, float4 fragPosViewSpace, float3 normal, float3 viewDir, float metalness, float linRoughness, float transmDist)
{
    float3 lDir = lightPosViewSpace - fragPosViewSpace;
    float dist = length(lDir);
    lDir = normalize(lDir);
    float3 halfVec = normalize(-normalize(fragPosViewSpace.xyz) + lDir.xyz);

    float attenuation = 1 / (pLight.constant + (pLight.lin * dist) + (pLight.quad * pow(dist, 2)));

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    float E = max(0.2f + dot(-normal, lDir), 0.0f);
    float3 transmittedRad = T(transmDist * 100.0f) * E;
    
    brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness);
    
    ExitRadiance_t exitRadiance;
    exitRadiance.diffuse = brdfOut.diffuse * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
    exitRadiance.specular = brdfOut.specular * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
    exitRadiance.transmitted = transmittedRad * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
    
    return exitRadiance;
}

ExitRadiance_t computeColSpotLight(int lightIndex, float4 posFragViewSpace, float3 normal, float metalness, float linRoughness, float4 posWS, uniform float4x4 lightViewMat, uniform float4x4 lightProjMat)
{
    ExitRadiance_t exitRadiance;
    exitRadiance.diffuse = float3(0.0f, 0.0f, 0.0f);
    exitRadiance.specular = float3(0.0f, 0.0f, 0.0f);
    exitRadiance.transmitted = float3(0.0f, 0.0f, 0.0f);

    float3 lightDir = normalize(posSpotLightViewSpace[lightIndex] - posFragViewSpace);
    float cosAngle = dot(-lightDir, normalize(dirSpotLightViewSpace[lightIndex].xyz));
    if (cosAngle > spotLights[lightIndex].cosCutoffAngle)
    {
        float3 viewDir = -normalize(posFragViewSpace).xyz;
        float3 halfVec = normalize(viewDir + lightDir);
            
        float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
        float LdotN = saturate(dot(lightDir, normal));
        float LdotH = saturate(dot(lightDir, halfVec));
        float NdotH = saturate(dot(halfVec, normal));
        
        //TODO: provide falloff for spotlight in constant buffer
        float attenuation = cosAngle - spotLights[lightIndex].cosCutoffAngle;
        
        float correctedBias = max(0.002f, 0.005f * (1.0f - abs(dot(lightDir, normal))));
        float transmDist = computeTransmDist(posWS, spotLights[lightIndex].nearPlaneDist, spotLights[lightIndex].farPlaneDist, lightViewMat, lightProjMat, correctedBias, SpotShadowMap, normal);
        float E = max(0.2f + dot(-normal, lightDir.xyz), 0.0f);
        float3 transmittedRad = T(transmDist * 100.0f) * E;
    
        brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness);
        exitRadiance.diffuse = brdfOut.diffuse * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
        exitRadiance.specular = brdfOut.specular * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
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
    float4 positionLPS = mul(shrinkedPos, mul(lightViewMat, lightProjMat));
    
    float3 ndCoords = positionLPS.xyz / positionLPS.w;
    float currentDepth = ndCoords.z; /*positionLPS.z; */
    float2 shadowMapUV = (ndCoords.xy + 1.0f) * rcp(2.0f);
    shadowMapUV = float2(shadowMapUV.x, 1.0f - shadowMapUV.y);
    
    float linSamplDepth = (nearPlane * farPlane) / (farPlane + /*min(*/shadowMap.Sample(texSampler, shadowMapUV).r/*, currentDepth-correctedBias)*/ * (nearPlane - farPlane));
    float linCurrDepth = (nearPlane * farPlane) / (farPlane + (currentDepth) * (nearPlane - farPlane));
    
    return abs(linSamplDepth - linCurrDepth);
}