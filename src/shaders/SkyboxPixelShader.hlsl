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
    
    output.col = cubemap.Sample(cubemapSampler, input.texCoord);
    
    return output;
}