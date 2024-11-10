static const float PI = 3.14159265f;
static const float HALF_PI = PI / 2.0f;
static const float QUARTER_PI = PI / 4.0f;

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

struct PS_OUTPUT
{
    float3 col : SV_TARGET;
};

TextureCube cubemap : register(t0);
SamplerState cubemapSampler : register(s0);

PS_OUTPUT ps(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float3 normal = normalize(input.texCoord);
    output.col = cubemap.Sample(cubemapSampler, normal).rgb;
    
    return output;
}