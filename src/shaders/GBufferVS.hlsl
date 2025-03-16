struct VS_INPUT
{
    float4 vPosition    : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 vNormal : NORMAL;
    float3 tangent : TANGENT0;
};

struct VS_OUTPUT
{
    float4 vPosition    : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 vNormal       : NORMAL;
    float3x3 tbn : TBNMATRIX;
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

    float3 t = normalize(mul(float4(Input.tangent, 0.0f), worldMatrix)).xyz;
    float3 n = normalize(mul(float4(Input.vNormal, 0.0f), worldMatrix)).xyz;
    float3 b = cross(t, n);

    Output.tbn = transpose(float3x3(t, b, n));
    
    float4x4 WVPMat = mul(worldMatrix, mul(viewMatrix, projMatrix));
    Output.vPosition = mul(Input.vPosition, WVPMat);

    Output.texCoord = Input.texCoord;
    Output.vNormal = mul(float4(Input.vNormal, 0.0f), worldMatrix/*mul(worldMatrix, viewMatrix)*/).xyz;
    
    return Output;
}