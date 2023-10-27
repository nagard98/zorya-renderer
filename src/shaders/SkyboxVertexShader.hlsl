struct VS_INPUT
{
    float4 pos : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 vNormal : NORMAL0;
};

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

VS_OUTPUT vs(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float4x4 WVPMat = mul(worldMatrix, mul(viewMatrix, projMatrix));
    float4 k = mul(input.pos, WVPMat);
    output.pos = float4(k.x, k.y, k.w, k.w);
    output.texCoord = input.pos.xyz;
    
    return output;
}
