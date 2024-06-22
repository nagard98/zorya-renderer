#include "common/LightStruct.hlsli"

#define CUBEMAP_FACE_POSITIVE_X 0.0f
#define CUBEMAP_FACE_NEGATIVE_X 1.0f
#define CUBEMAP_FACE_POSITIVE_Y 2.0f
#define CUBEMAP_FACE_NEGATIVE_Y 3.0f
#define CUBEMAP_FACE_POSITIVE_Z 4.0f
#define CUBEMAP_FACE_NEGATIVE_Z 5.0f

static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f / 2.2f;
static const float lightDec = -(1 / 16.0f);
static const float texScale = 1 / 4096.0f;
//static const float SCREEN_WIDTH = 1280.0f;
//static const float SCREEN_HEIGHT = 720.0f;
static const float SCREEN_WIDTH = 1920.0f;
static const float SCREEN_HEIGHT = 1080.0f;

struct PS_OUT
{
    float4 diffuseShadowed : SV_Target0;
    float4 specularShadowed : SV_Target1;
};


cbuffer light : register(b0)
{
    Directional_Light dirLight;
    
    int numPLights;
    int numSpotLights;
    int2 pad;
    
    Point_Light pointLights[16];
    float4 posPointLightViewSpace[16];
    
    Spot_Light spotLights[16];
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

Texture2D diffuse : register(t0);
Texture2D gbuffer_normal : register(t1);
Texture2D specular : register(t2);
Texture2D gbuffer_depth : register(t3);

Texture2D ShadowMap : register(t4);
Texture2DArray ShadowCubeMap : register(t5);
//TextureCube ShadowCubeMap : register(t5);
Texture2D SpotShadowMap : register(t6);

Texture2D gbuffer_albedo : register(t7);
Texture2D transmitted_irrad : register(t8);

SamplerState texSampler : register(s0);

float computeShadowing(float4 posLightSpace, float bias, float NdotL);
float computeSpotShadowing(int lightIndex, float4 posWorldSpace, float4 posFragViewSpace, float3 normalViewSpace, float minBias, float maxBias);
float computeOmniShadowing(int lightIndex, float4 posWorldSpace, float4 posViewSpace, float3 normalViewSpace, float minBias, float maxBias);

float4 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat);

PS_OUT ps(float4 posFragQuad : SV_POSITION) : SV_Target
{
    float2 uvCoord = float2(posFragQuad.x / SCREEN_WIDTH, posFragQuad.y / SCREEN_HEIGHT);
    
    float4 diffuseCol = diffuse.Sample(texSampler, uvCoord);
    float4 ambCol = float4(0.03f, 0.03f, 0.03f, 1.0f); // * gbuffer_albedo.Sample(texSampler, uvCoord);
    float3 trans_irrad = transmitted_irrad.Sample(texSampler, uvCoord).rgb;
    float4 specCol = specular.Sample(texSampler, uvCoord);
    float sampledDepth = gbuffer_depth.Sample(texSampler, uvCoord);
    float3 normal = gbuffer_normal.Sample(texSampler, uvCoord).xyz * 2.0f - 1.0f;
    
    float4 posVS = posFromDepth(uvCoord, sampledDepth, invCamProjMat);
    float4 posWS = mul(posVS, invCamViewMat);
    
    float totalArrivingLight = 0.0f;
    
    //TODO : use stencil buffer for to exclude skybox, instead of branching
    //if (sampledDepth < 0.999f)
    float NdotL = 0.0f;
    
    float totalNumLights = numPLights + numSpotLights;
    
    if (dirLight.dir.w == 0.0f)
    {
        NdotL = dot(normal, -dirLight.dir.xyz);
        totalArrivingLight += computeShadowing(posWS, 0.006f, NdotL);
        totalNumLights += 1.0f;
    }

    for (int i = 0; i < numSpotLights; i++)
    {
        totalArrivingLight += computeSpotShadowing(i, posWS, posVS, normal, 0.003f, 0.009f);
    }
    
    for (int j = 0; j < numPLights; j++)
    {
        totalArrivingLight += computeOmniShadowing(j, posWS, posVS, normal, 0.003f, 0.009f);
    }
    
    PS_OUT ps_out;
    ps_out.diffuseShadowed = float4(diffuseCol.rgb * (totalArrivingLight * rcp(totalNumLights)) + ambCol.rgb + trans_irrad, 1.0f);
    ps_out.specularShadowed = float4(specCol.rgb * (totalArrivingLight * rcp(totalNumLights)), 1.0f);
    
    return ps_out;
}

float computeShadowing(float4 posWorldSpace, float bias, float NdotL)
{
    float4 posLightSpace = mul(posWorldSpace, mul(dirLightViewMat, dirLightProjMat));
    
    float correctedBias = max(0.001f, bias * (1.0f - abs(NdotL)));
    float3 ndCoords = posLightSpace.xyz / posLightSpace.w;
    float currentDepth = ndCoords.z;
    float2 shadowMapUV = ndCoords.xy * 0.5f + 0.5f;
    shadowMapUV = float2(shadowMapUV.x, 1.0f - shadowMapUV.y);
    
    float shadowingSampled = 1.0f;
    //Brute force 16 samples
    for (float i = -1.5; i <= 1.5; i += 1.0f)
    {
        for (float j = -1.5; j <= 1.5; j += 1.0f)
        {
            float2 offset = float2(i, j) * texScale;
            float sampledDepth = ShadowMap.Sample(texSampler, shadowMapUV + offset).r;
            shadowingSampled += sampledDepth > currentDepth + correctedBias ? lightDec : 0.0f;
        }
    }
    
    return shadowingSampled;
}

float computeSpotShadowing(int lightIndex, float4 posWorldSpace, float4 posFragViewSpace, float3 normalViewSpace, float minBias, float maxBias)
{
    float3 lightDir = normalize(posSpotLightViewSpace[lightIndex] - posFragViewSpace);
    float cosAngle = dot(-lightDir, normalize(dirSpotLightViewSpace[lightIndex].xyz));
    
    float NdotL = dot(normalViewSpace, lightDir);
    float correctedBias = max(minBias, maxBias * (1.0f - abs(NdotL)));
    
    float shadowingSampled = 1.0f;
    
    if (cosAngle > spotLights[lightIndex].cos_cutoff_angle)
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
                    float sampledDepth = SpotShadowMap.Sample(texSampler, shadowMapUVF + offset).r;
                    shadowingSampled += sampledDepth < currentDepth - correctedBias ? lightDec : 0.0f;
                }
            }
        }
    }
    
    return shadowingSampled;
}

float computeOmniShadowing(int lightIndex, float4 posWorldSpace, float4 posViewSpace, float3 normalViewSpace, float minBias, float maxBias)
{
    float3 lightDir = normalize(posPointLightViewSpace[lightIndex] - posViewSpace);
    float LdotN = saturate(dot(lightDir, normalViewSpace));
    float correctedBias = max(minBias, maxBias * (1.0f - LdotN));

    float shadowingSampled = 1.0f;
    
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
                    float sampledDepth = ShadowCubeMap.Sample(texSampler, shadowMapUVF + offset).r;
                    shadowingSampled += sampledDepth < currentDepth - correctedBias ? lightDec : 0.0f;
                }
            }
                
        }
    }
    
    return shadowingSampled;
}

float4 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat)
{
    float x = quadTexCoord.x * 2.0f - 1.0f;
    float y = (1.0f - quadTexCoord.y) * 2.0f - 1.0f;
    
    float4 vProjPos = float4(x, y, sampledDepth, 1.0f);
    float4 vPosVS = mul(vProjPos, invMat);
    
    return vPosVS.xyzw / vPosVS.w;
}