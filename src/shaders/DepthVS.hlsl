struct VS_INPUT
{
    float4 vPosition : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 vNormal : NORMAL;
    float3 tangent : TANGENT0;
};

struct VS_OUTPUT
{
    float4 lightSpacePos : SV_Position;
};

cbuffer cbPerObj : register(b0)
{
    float4x4 worldMatrix;
};

cbuffer cbPerCam : register(b1)
{
    float4x4 lightViewMat;
};

cbuffer cbPerProj : register(b2)
{
    float4x4 lightProjMat;
};


VS_OUTPUT vs(VS_INPUT input)
{
    VS_OUTPUT output;
    output.lightSpacePos = mul(input.vPosition, mul(worldMatrix, mul(lightViewMat, lightProjMat)));
    
    return output;
}