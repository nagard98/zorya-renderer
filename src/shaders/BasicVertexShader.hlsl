struct VS_INPUT
{
    float4 vPosition    : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 vNormal : NORMAL0;
};

struct VS_OUTPUT
{
    float4 vPosition    : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 vNormal       : NORMAL0;
    float4 posViewSpace : POSITION0;
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

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT vs(VS_INPUT Input)
{
    VS_OUTPUT Output;

    Output.posViewSpace = mul(Input.vPosition, mul(worldMatrix, viewMatrix));
    float4x4 WVPMat = mul(worldMatrix, mul(viewMatrix, projMatrix));
    Output.vPosition = mul(Input.vPosition, WVPMat);
    Output.texCoord = Input.texCoord;
    Output.vNormal = mul(Input.vNormal, mul(worldMatrix, viewMatrix));
    
    return Output;
}