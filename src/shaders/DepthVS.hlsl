#define DIRECTIONAL_LIGHT_ID 0
#define POINT_LIGHT_ID 1
#define SPOT_LIGHT_ID 2

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

//cbuffer cbPerCam : register(b1)
//{
//    float4x4 lightViewMat;
//};

//cbuffer cbPerProj : register(b2)
//{
//    float4x4 lightProjMat;
//};

cbuffer omniDirShadowMatrixes : register(b2)
{
    float4x4 dirLightViewMat[1];
    float4x4 dirLightProjMat[1];
    
    float4x4 lightViewMat[16 * 6];
    float4x4 lightProjMat;
    
    float4x4 spotLightViewMat[16];
    float4x4 spotLightProjMat[16];
}

cbuffer draw_constants : register(b3)
{
    uint light_index;
    uint light_type;
    uint face;
}

VS_OUTPUT vs(VS_INPUT input)
{
    VS_OUTPUT output;
    //output.lightSpacePos = mul(input.vPosition, mul(worldMatrix, mul(lightViewMat, lightProjMat)));
    switch (light_type)
    {
        case DIRECTIONAL_LIGHT_ID : {
                output.lightSpacePos = mul(input.vPosition, mul(worldMatrix, mul(dirLightViewMat[light_index], dirLightProjMat[light_index])));
                break;
            }
        case SPOT_LIGHT_ID : {
                output.lightSpacePos = mul(input.vPosition, mul(worldMatrix, mul(spotLightViewMat[light_index], spotLightProjMat[light_index])));
                break;
            }
        case POINT_LIGHT_ID : {
                output.lightSpacePos = mul(input.vPosition, mul(worldMatrix, mul(lightViewMat[light_index * 6 + face], lightProjMat)));
                break;
            }

    };
    
    return output;
}