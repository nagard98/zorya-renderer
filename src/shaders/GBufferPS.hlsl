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
    float3x3 tbn : TBNMATRIX;
};

struct PS_OUTPUT
{
    float4 albedo : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 roughMet: SV_TARGET2;
};


cbuffer matPrms : register(b1)
{
    float4 samples[16];
    float4 baseColor;
	float4 subsurfaceAlbedo;
	float4 meanFreePathColor;
    bool hasAlbedoMap;
    bool hasMetalnessMap;
    bool hasNormalMap;
    bool hasSmoothnessMap;
    
    float cb_roughness;
    float cb_metallic;
	float meanFreePathDist;
	float scale;
    
}

#define MAX_SHININESS 64
#define SPECULAR_STRENGTH 0.3f
#define INNER_REFLECTANCE 0.5f

static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f / 2.2f;
static const float lightDec = -(1 / 16.0f);
static const float texScale = 1 / 2048.0f;

Texture2D ObjTexture : register(t0);
Texture2D NormalMap : register(t1);
Texture2D MetalnessMap : register(t2);
Texture2D SmoothnessMap : register(t3);

Texture2D ShadowMap : register(t4);
Texture2DArray ShadowCubeMap : register(t5);
//TextureCube ShadowCubeMap : register(t5);
Texture2D SpotShadowMap : register(t6);

SamplerState ObjSamplerState : register(s0);


PS_OUTPUT ps(PS_INPUT input)
{
    PS_OUTPUT output;

    if (hasNormalMap == true)
    {
        input.fNormal = normalize(NormalMap.Sample(ObjSamplerState, input.texCoord).rgb * 2.0f - 1.0f);
        output.normal = float4(mul(input.tbn, input.fNormal), 0.0f);
        output.normal = output.normal * 0.5f + 0.5f;
    }
    else
    {
        output.normal = float4(normalize(input.fNormal), 0.0f);
        output.normal = output.normal * 0.5f + 0.5f;
    }
    
    output.roughMet.g = cb_metallic;
    if (hasMetalnessMap == true)
    {
        output.roughMet.g =  MetalnessMap.Sample(ObjSamplerState, input.texCoord).r;
    }
    
    
    output.roughMet.r = cb_roughness;
    if (hasSmoothnessMap == true)
    {
        output.roughMet.r = SmoothnessMap.Sample(ObjSamplerState, input.texCoord).g;
    }    
    
    output.albedo = baseColor;
    if (hasAlbedoMap == true)
    {
        output.albedo *= ObjTexture.Sample(ObjSamplerState, input.texCoord);
    }
    
    return output;
}


