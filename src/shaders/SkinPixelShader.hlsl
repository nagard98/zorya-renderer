#include "BRDF.hlsl"

#define CUBEMAP_FACE_POSITIVE_X 0.0f
#define CUBEMAP_FACE_NEGATIVE_X 1.0f
#define CUBEMAP_FACE_POSITIVE_Y 2.0f
#define CUBEMAP_FACE_NEGATIVE_Y 3.0f
#define CUBEMAP_FACE_POSITIVE_Z 4.0f
#define CUBEMAP_FACE_NEGATIVE_Z 5.0f

struct PS_INPUT
{
    float4 fPos : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 fNormal : NORMAL;
    float4 posViewSpace : POSITION0;
    float4 posLightSpace : POSITION1;
    float4 posWorldSpace : POSITION2;
    float3x3 tbn : TBNMATRIX;
};

struct PS_OUTPUT
{
    float4 diffuse : SV_TARGET0;
    float4 specular : SV_TARGET1;
};

struct DirectionalLight
{
    float4 dir;
};

struct PointLight
{
    float4 posWorldSpace;
    
    float constant;
    float lin;
    float quad;
};

struct SpotLight
{
    float4 posWorldSpace;
    float4 direction;
    float cosCutoffAngle;
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
    PointLight pointLights[16];
    float4 posPointLightViewSpace[16];
    
    int numSpotLights;
    SpotLight spotLights[16];
    float4 posSpotLightViewSpace[16];
    float4 dirSpotLightViewSpace[16];
};

cbuffer matPrms : register(b1)
{
    float4 baseColor;
    bool hasAlbedoMap;
    bool hasMetalnessMap;
    bool hasNormalMap;
    bool hasSmoothnessMap;
    
    float cb_roughness;
    float cb_metallic;
}

cbuffer omniDirShadowMatrixes : register(b2)
{
    float4x4 lightViewMat[16 * 6];
    float4x4 lightProjMat;
    
    float4x4 spotLightViewMat[16];
    float4x4 spotLightProjMat[16];
}

#define MAX_SHININESS 64
#define SPECULAR_STRENGTH 0.3f
#define INNER_REFLECTANCE 0.5f

static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f / 2.2f;
static const float lightDec = -(1 / 16.0f);
static const float texScale = 1 / 2048.0f;

static const float zf = 20.0f;
static const float zn = 2.0f;
static const float q = zf / (zf - zn);

Texture2D ObjTexture : register(t0);
Texture2D NormalMap : register(t1);
Texture2D MetalnessMap : register(t2);
Texture2D SmoothnessMap : register(t3);
Texture2D ShadowMap : register(t4);
Texture2DArray ShadowCubeMap : register(t5);
//TextureCube ShadowCubeMap : register(t5);
Texture2D SpotShadowMap : register(t6);

SamplerState ObjSamplerState : register(s0);

ExitRadiance_t computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex, float metalness, float roughness, float transmDist);
ExitRadiance_t computeColPointLight(PointLight pLight, float4 posLightViewSpace, float4 fragPosViewSpace, float3 normal, float3 viewDir, float4 tex, float metalness, float roughness, float transmDist);
ExitRadiance_t computeColSpotLight(int lightIndex, float4 posFragViewSpace, float3 normal, float metalness, float roughness, float transmDist);

float computeShadowing(float4 posLightSpace, float bias, float NdotL, out float transmDist);
float computeSpotShadowing(int lightIndex, float4 posWorldSpace, float4 posFragViewSpace, float minBias, float maxBias, out float transmDist);
float computeOmniShadowing(int lightIndex, float4 posWorldSpace, float4 posViewSpace, float3 normalViewSpace, float minBias, float maxBias, out float transmDist);

float3 T(float s);

PS_OUTPUT ps(PS_INPUT input)
{
    PS_OUTPUT output;

    if (hasNormalMap == true)
    {
        input.fNormal = normalize(NormalMap.Sample(ObjSamplerState, input.texCoord).rgb * 2.0f - 1.0f);
        input.fNormal = mul(input.tbn, input.fNormal);
    }
    else
    {
        input.fNormal = normalize(input.fNormal);
    }
    
    float metalness = cb_metallic;
    if (hasMetalnessMap == true)
    {
        metalness = MetalnessMap.Sample(ObjSamplerState, input.texCoord).r;
    }
    
    float roughness = cb_roughness;
    if (hasSmoothnessMap == true)
    {
        roughness = SmoothnessMap.Sample(ObjSamplerState, input.texCoord).g;
    }
    float linRoughness = pow(roughness, 4.0f);
    
    float4 tex = 0.0f;
    if (hasAlbedoMap == true)
    {
        tex = ObjTexture.Sample(ObjSamplerState, input.texCoord) * pow(baseColor, 1.0f / gamma);
    }
    else
    {
        tex = pow(baseColor, 1.0f / gamma);
    }
    
    float3 viewDir = -normalize(input.posViewSpace).xyz;
    
    float shadowing = 0.0f;
    
    ExitRadiance_t radExitance;
    radExitance.diffuse = float3(0.0f, 0.0f, 0.0f);
    radExitance.specular = float3(0.0f, 0.0f, 0.0f);

    if (dirLight.dir.w == 0.0f)
    {
        float transmDist = 0.0f;
        shadowing = computeShadowing(input.posLightSpace, 0.005f, dot(input.fNormal, -dirLight.dir.xyz), transmDist);
        ExitRadiance_t tmpRadExitance = computeColDirLight(dirLight, viewDir, input.fNormal, tex, metalness, linRoughness, transmDist);

        radExitance.diffuse += (tmpRadExitance.diffuse * shadowing) + tmpRadExitance.transmitted;
        radExitance.specular += tmpRadExitance.specular * shadowing;
    }
    
    for (int i = 0; i < numSpotLights; i++)
    {
        float transmDist = 0.0f;
        shadowing = computeSpotShadowing(i, input.posWorldSpace, input.posViewSpace, 0.005f, 0.01f, transmDist);
        ExitRadiance_t tmpRadExitance = computeColSpotLight(i, input.posViewSpace, input.fNormal, metalness, linRoughness, transmDist);
        
        radExitance.diffuse += tmpRadExitance.diffuse * shadowing;
        radExitance.specular += tmpRadExitance.specular * shadowing;
    }
    
    for (int i = 0; i < numPLights; i++)
    {
        float transmDist = 0.0f;
        shadowing = computeOmniShadowing(i, input.posWorldSpace, input.posViewSpace, input.fNormal, 0.005f, 0.01f, transmDist);
        ExitRadiance_t tmpRadExitance = computeColPointLight(pointLights[i], posPointLightViewSpace[i], input.posViewSpace, input.fNormal, viewDir, tex, metalness, linRoughness, transmDist);
        
        radExitance.diffuse += tmpRadExitance.diffuse * shadowing;
        radExitance.specular += tmpRadExitance.specular * shadowing;
    }
    
    float3 ambCol = 0.02f * tex.rgb;

    output.diffuse = float4(radExitance.diffuse * tex.rgb + ambCol, 1.0f);
    output.specular = float4(radExitance.specular, 1.0f);
    
    return output;
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

float computeShadowing(float4 posLightSpace, float bias, float NdotL, out float transmDist)
{
    float correctedBias = max(0.002f, bias * (1.0f - abs(NdotL)));
    float3 ndCoords = posLightSpace.xyz / posLightSpace.w;
    float currentDepth = ndCoords.z;
    float2 shadowMapUV = ndCoords.xy * 0.5f + 0.5f;
    shadowMapUV = float2(shadowMapUV.x, 1.0f - shadowMapUV.y);
    
    float linSamplDepth = (zn * zf) / (zf + min(ShadowMap.Sample(ObjSamplerState, shadowMapUV).r, currentDepth - correctedBias) * (zn - zf));
    float linCurrDepth = (zn * zf) / (zf + (currentDepth) * (zn - zf));
    transmDist = abs(linSamplDepth - linCurrDepth);
    
    float shadowingSampled = 1.0f;
    //Brute force 16 samples
    for (float i = -1.5; i <= 1.5; i += 1.0f)
    {
        for (float j = -1.5; j <= 1.5; j += 1.0f)
        {
            float2 offset = float2(i, j) * texScale;
            float sampledDepth = ShadowMap.Sample(ObjSamplerState, shadowMapUV + offset).r;
            shadowingSampled += sampledDepth < currentDepth - correctedBias ? lightDec : 0.0f;
        }
    }
    
    return shadowingSampled;
}

float computeSpotShadowing(int lightIndex, float4 posWorldSpace, float4 posFragViewSpace, float minBias, float maxBias, out float transmDist)
{
    float shadowingSampled = 1.0f;
    float correctedBias = 0.008f; // max(0.001f, bias * (1.0f - abs(NdotL)));
    
    transmDist = 0.0f;
        
    float3 lightDir = normalize(posSpotLightViewSpace[lightIndex] - posFragViewSpace);
    float cosAngle = dot(-lightDir, normalize(dirSpotLightViewSpace[lightIndex].xyz));
    if (cosAngle > spotLights[lightIndex].cosCutoffAngle)
    {
        float4 posLightSpace = mul(posWorldSpace, mul(spotLightViewMat[lightIndex], spotLightProjMat[lightIndex]));
        float3 ndcPosLightSpace = (posLightSpace.xyz / posLightSpace.w);
        
        if (ndcPosLightSpace.x < 1.0f && ndcPosLightSpace.y < 1.0f && ndcPosLightSpace.z < 1.0f &&
            ndcPosLightSpace.x > -1.0f && ndcPosLightSpace.y > -1.0f && ndcPosLightSpace.z > 0.0f)
        {
            //float3 sampleDir = posWorldSpace - float4(pointLights[0].posWorldSpace.x, 0.0f, 0.0f, 1.0f);
            float2 normCoords = (ndcPosLightSpace.xy * 0.5f) + 0.5f;
                
            //DirectX maps the near plane to 0, not -1
            float currentDepth = ndcPosLightSpace.z;
            float2 shadowMapUVF = float2(normCoords.x, 1.0f - normCoords.y);
    
            //Brute force 16 samples
            for (float i = -1.5; i <= 1.5; i += 1.0f)
            {
                for (float j = -1.5; j <= 1.5; j += 1.0f)
                {
                    float2 offset = float2(i, j) * texScale;
                    float sampledDepth = SpotShadowMap.Sample(ObjSamplerState, shadowMapUVF + offset).r;
                    shadowingSampled += sampledDepth < currentDepth - correctedBias ? lightDec : 0.0f;
                }
            }
        }
    }
    
    return shadowingSampled;
}

float computeOmniShadowing(int lightIndex, float4 posWorldSpace, float4 posViewSpace, float3 normalViewSpace, float minBias, float maxBias, out float transmDist)
{
    transmDist = 0.0f;
    float shadowingSampled = 1.0f;

    float3 lightDir = normalize(posPointLightViewSpace[lightIndex] - posViewSpace);
    float LdotN = saturate(dot(lightDir, normalViewSpace));
    float correctedBias = max(minBias, maxBias * (1.0f - LdotN));
        
    int cubemapOffset = lightIndex * 6;
        
    for (float face = 0; face < 6; face += 1.0f)
    {
        float4 posLightSpace = mul(posWorldSpace, mul(lightViewMat[face + cubemapOffset], lightProjMat));
        float3 ndcPosLightSpace = (posLightSpace.xyz / posLightSpace.w);
        
        if (ndcPosLightSpace.x < 1.0f && ndcPosLightSpace.y < 1.0f && ndcPosLightSpace.z < 1.0f &&
        ndcPosLightSpace.x > -1.0f && ndcPosLightSpace.y > -1.0f && ndcPosLightSpace.z > 0.0f)
        {
            //float3 sampleDir = posWorldSpace - float4(pointLights[0].posWorldSpace.x, 0.0f, 0.0f, 1.0f);
            float2 normCoords = (ndcPosLightSpace.xy * 0.5f) + 0.5f;
                
            //DirectX maps the near plane to 0, not -1
            float currentDepth = ndcPosLightSpace.z;
            float3 shadowMapUVF = float3(normCoords.x, 1.0f - normCoords.y, face + cubemapOffset);
    
            //Brute force 16 samples
            for (float i = -1.5; i <= 1.5; i += 1.0f)
            {
                for (float j = -1.5; j <= 1.5; j += 1.0f)
                {
                    float3 offset = float3(i, j, 0.0f) * texScale;
                    float sampledDepth = ShadowCubeMap.Sample(ObjSamplerState, shadowMapUVF + offset).r;
                    shadowingSampled += sampledDepth < currentDepth - correctedBias ? lightDec : 0.0f;
                }
            }
                
        }
    }
    
    return shadowingSampled;

}

ExitRadiance_t computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex, float metalness, float linRoughness, float transmDist)
{
    ExitRadiance_t exitRadiance;
    float3 lDir = -normalize(dLight.dir.xyz);
    //float3 lRef = reflect(lDir, normal);
    
    float3 halfVec = normalize(lDir + viewDir);

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    float E = max(0.2f + dot(-normal, -dLight.dir.xyz), 0.0f);
    //TODO: instead of hardcoded scale, use model scale
    float3 transmittedRad = T(transmDist * 100.0f) * E;
    
    brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness);
    exitRadiance.diffuse = brdfOut.diffuse *LdotN * float3(3.14f, 3.14f, 3.14f) * revPi;
    exitRadiance.specular = brdfOut.specular * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi;
    exitRadiance.transmitted = transmittedRad * float3(3.14f, 3.14f, 3.14f) * revPi;
    
    return exitRadiance;
}

ExitRadiance_t computeColPointLight(PointLight pLight, float4 lightPosViewSpace, float4 fragPosViewSpace, float3 normal, float3 viewDir, float4 tex, float metalness, float linRoughness, float transmDist)
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

ExitRadiance_t computeColSpotLight(int lightIndex, float4 posFragViewSpace, float3 normal, float metalness, float linRoughness, float transmDist)
{
    ExitRadiance_t exitRadiance;
    exitRadiance.diffuse = float3(0.0f, 0.0f, 0.0f);
    exitRadiance.specular = float3(0.0f, 0.0f, 0.0f);

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
        
        float E = max(0.2f + dot(-normal, lightDir.xyz), 0.0f);
        float3 transmittedRad = T(transmDist * 100.0f) * E;
    
        brdfOut_t brdfOut = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness);
        exitRadiance.diffuse = (brdfOut.diffuse * LdotN + transmittedRad) * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
        exitRadiance.specular = brdfOut.specular * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
        exitRadiance.transmitted = transmittedRad * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
    }
    
    return exitRadiance;

}


