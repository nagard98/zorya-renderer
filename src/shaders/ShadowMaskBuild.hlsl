#include "common/LightStruct.hlsli"

#define SCREEN_WIDTH 1920.0f
#define SCREEN_HEIGHT 1080.0f

#define DIRECTIONAL_LIGHT_ID 0
#define POINT_LIGHT_ID 1
#define SPOT_LIGHT_ID 2

static const int numSamplesJimenezSep = 15;
static const float lightDec = -(1 / 16.0f);
static const float texScale = 1 / 4096.0f;

Texture2D scene_depth : register(t0);
Texture2D gbuffer_normal : register(t1);
Texture2D shadow_map : register(t2);
Texture2DArray shadow_cubemap : register(t3);

SamplerState tex_sampler : register(s0);

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
    Directional_Light dirLight;
    
    int numPLights;
    int numSpotLights;
    int2 pad;
    
    Point_Light Point_Lights[16];
    float4 posPointLightViewSpace[16];
    
    Spot_Light spotLights[16];
    float4 posSpotLightViewSpace[16];
    float4 dirSpotLightViewSpace[16];
    
    ModelSSS sssModels[64];
};

cbuffer cam_inverse_matrix : register(b1)
{
    float4x4 inv_cam_proj_matrix;
    float4x4 inv_cam_view_matrix;
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

cbuffer draw_constants : register(b3)
{
    uint light_index;
    uint light_type;
}

float computeShadowing(float4 posLightSpace, float bias, float NdotL);
float computeSpotShadowing(int lightIndex, float4 posWorldSpace, float4 posFragViewSpace, float3 normalViewSpace, float minBias, float maxBias);
float computeOmniShadowing(int lightIndex, float4 posWorldSpace, float4 posViewSpace, float3 normalViewSpace, float minBias, float maxBias);

float4 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat);

float4 ps(float4 frag_pos : SV_POSITION) : SV_Target0
{
    float2 uvCoord = float2(frag_pos.x / SCREEN_WIDTH, frag_pos.y / SCREEN_HEIGHT);
    
    float sampledDepth = scene_depth.Sample(tex_sampler, uvCoord).r;
    float3 normal = gbuffer_normal.Sample(tex_sampler, uvCoord).xyz * 2.0f - 1.0f;
    
    float4 posVS = posFromDepth(uvCoord, sampledDepth, inv_cam_proj_matrix);
    float4 posWS = mul(posVS, inv_cam_view_matrix);
                
    float shadowing_factor = 1.0f;
    
    //if (dirLight.dir.w == 0.0f)
    //{
    
    //TODO: fix; use masking (need to make dsv read-only as its used as stencil and as shader resource to determin depth)
    if (sampledDepth < 0.9999)
    {
        switch (light_type)
        {
            case DIRECTIONAL_LIGHT_ID:
                {
                    float NdotL = dot(normal, -dirLight.dir.xyz);
                    shadowing_factor = computeShadowing(posWS, 0.006f, NdotL);
                    break;
                }
            case POINT_LIGHT_ID:
                {
                    shadowing_factor = computeOmniShadowing(light_index, posWS, posVS, normal, 0.003f, 0.009f);
                    break;
                }
            case SPOT_LIGHT_ID:
                {
                    shadowing_factor = computeSpotShadowing(light_index, posWS, posVS, normal, 0.003f, 0.009f);
                    break;
                }
        }
    }

    //for (int i = 0; i < numSpotLights; i++)
    //{
    //    totalArrivingLight += computeSpotShadowing(i, posWS, posVS, normal, 0.003f, 0.009f);
    //}
    
    //for (int j = 0; j < numPLights; j++)
    //{
    //    totalArrivingLight += computeOmniShadowing(j, posWS, posVS, normal, 0.003f, 0.009f);
    //}
    
    return float4(shadowing_factor, 0, 0, 0);
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
            float sampledDepth = shadow_map.SampleLevel(tex_sampler, shadowMapUV + offset, 0).r;
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
                    float sampledDepth = shadow_map.Sample(tex_sampler, shadowMapUVF + offset).r;
                    shadowingSampled += sampledDepth > currentDepth + correctedBias ? lightDec : 0.0f;
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
                    float sampledDepth = shadow_cubemap.Sample(tex_sampler, shadowMapUVF + offset).r;
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