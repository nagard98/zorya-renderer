static const float PI = 3.14159265f;
static const float HALF_PI = 3.14159265f / 2.0f;
static const float QUARTER_PI = 3.14159265f / 4.0f;

struct PS_INPUT
{
    float4 pos : SV_Position;
    float3 tex_coord : TEXCOORD0;
};

Texture2D equirectangular_map : register(t0);

SamplerState equirectangular_sampler : register(s0);


float4 ps(PS_INPUT input) : SV_Target
{
    float3 pos = normalize(input.tex_coord.xyz);
    
    float theta = atan2(pos.z, pos.x);
    float phi = asin(-pos.y);
        
    float2 uv = float2((theta + PI) / (2 * PI), (phi + HALF_PI) / PI);

    return equirectangular_map.Sample(equirectangular_sampler, uv);
}