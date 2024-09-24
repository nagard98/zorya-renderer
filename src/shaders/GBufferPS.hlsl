#include "common/BRDF.hlsli"

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
    float4 indirect_light : SV_TARGET0;
    float4 albedo : SV_TARGET1;
    float2 normal : SV_TARGET2;
    float4 roughMet: SV_TARGET3;
    float4 vertNormal : SV_TARGET4;
    float4 translucent : SV_TARGET5;
};

cbuffer _matPrms : register(b1)
{
    float4 _baseColor = float4(0.9f, 0.9f, 0.9f, 1.0f);

    bool hasAlbedoMap = false;
    bool hasMetalnessMap = false;
    bool hasNormalMap = false;
    bool hasSmoothnessMap = false;
    
    float _cb_roughness = 0.5f;
    float _cb_metallic = 0.0f;   
    
    float2 dir = float2(1.0f, 0.0f);
    uint _sssModelId;
    uint _sssModelArrIndex;
    float pad;
    float pad2;
}

#define MAX_SHININESS 64
#define SPECULAR_STRENGTH 0.3f
#define INNER_REFLECTANCE 0.5f

static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f / 2.2f;
static const float lightDec = -(1 / 16.0f);
static const float texScale = 1 / 2048.0f;
static const float ambient_factor = 0.05f;

Texture2D _ObjTexture : register(t0);
Texture2D _NormalMap : register(t1);
Texture2D _MetalnessMap : register(t2);
Texture2D _SmoothnessMap : register(t3);
Texture2D ThicknessMap : register(t4);

SamplerState ObjSamplerState : register(s0);


float2 oct_wrap(float2 v)
{
    return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}
 
float2 encode_normal(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : oct_wrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}

PS_OUTPUT ps(PS_INPUT input)
{
    PS_OUTPUT output;
    output.vertNormal = float4(normalize(input.fNormal), 0.0f) * 0.5f + 0.5f;

    if (hasNormalMap == true)
    {
        input.fNormal = normalize(_NormalMap.Sample(ObjSamplerState, input.texCoord).rgb * 2.0f - 1.0f);
        output.normal = encode_normal(mul(input.tbn, input.fNormal));
    }
    else
    {
        output.normal = encode_normal(normalize(input.fNormal));
    }
    //output.normal = output.normal * 0.5f + 0.5f;
    
    output.roughMet.g = _cb_metallic;
    
    if (hasMetalnessMap == true)
    {
        output.roughMet.g =  _MetalnessMap.Sample(ObjSamplerState, input.texCoord).r;
    }
    
    
    output.roughMet.r = _cb_roughness;
    if (hasSmoothnessMap == true)
    {
        output.roughMet.r = _SmoothnessMap.Sample(ObjSamplerState, input.texCoord).g;
    }    
    
    output.roughMet.b = ThicknessMap.Sample(ObjSamplerState, input.texCoord).r;
    
    output.albedo = _baseColor;
    if (hasAlbedoMap == true)
    {
        output.albedo *= _ObjTexture.Sample(ObjSamplerState, input.texCoord);
    }
    
    int subsurfaceParamsPack = ((_sssModelArrIndex & 63)) + ((_sssModelId & 3) << 6);
    output.albedo.a = (float)subsurfaceParamsPack / 255.0f;
    
    output.indirect_light = output.albedo * ambient_factor;
    
    return output;
}


