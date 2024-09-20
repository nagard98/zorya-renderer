struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 col : SV_TARGET;
};

TextureCube cubemap : register(t0);
SamplerState cubemapSampler : register(s0);

PS_OUTPUT ps(PS_INPUT input)
{
    PS_OUTPUT output;
    
    output.col = pow(cubemap.Sample(cubemapSampler, input.texCoord), 2.2f);
    //output.col = float4(0.0f,0.0f,0.0f,1.0f);
    
    return output;
}