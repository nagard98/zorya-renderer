#include "common/CubeData.hlsli"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

cbuffer cbPerObj : register(b0)
{
    float4x4 worldMatrix;
};

cbuffer cbPerCam : register(b1)
{
    float4x4 viewMatrix;
};

cbuffer cbPerProj : register(b2)
{
    float4x4 projMatrix;
};

VS_OUTPUT vs(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    
    float4x4 WVPMat = mul(worldMatrix, mul(viewMatrix, projMatrix));
    float4 k = mul(CUBE[vertexID], WVPMat);
    output.pos = float4(k.x, k.y, k.w, k.w);
    output.texCoord = CUBE[vertexID].xyz;
    
    return output;
}
