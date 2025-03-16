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


float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ToneMapReinhard(float3 hdrColor)
{
    return hdrColor / (hdrColor + 1.0);
}

float3 ToneMapACES(float3 x)
{
    // Apply the ACES filmic curve
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

PS_OUTPUT ps(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float3 normal = normalize(input.texCoord);
    
    float sample_delta = 0.015f;
    float num_samples = 4096.0f;
    float3 accum = float3(0.0f, 0.0f, 0.0f);
    
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = normalize(cross(normal, tangent));
    
    float3x3 tnb = transpose(float3x3(tangent, normal, bitangent));
    
    
    for (int i = 0; i < num_samples; i++)
    {
        float2 uv = Hammersley(i, num_samples);
        float sample_theta = 2 * PI * uv.x;
        float sample_phi = asin(sqrt(uv.y));
        float pdf = (1 / PI) * cos(sample_phi) * sin(sample_phi);
        
        
        float3 tangent_space_sample = normalize(float3(sin(sample_phi) * cos(sample_theta), cos(sample_phi), sin(sample_phi) * sin(sample_theta)));
        float3 world_space_normal = normalize(mul(tnb, tangent_space_sample));
            
        accum += (ToneMapACES(cubemap.Sample(cubemapSampler, world_space_normal).rgb) * cos(sample_phi) * sin(sample_phi)) / pdf;
        
    }
    
    //for (float sample_theta = 0.0f; sample_theta < 2 * PI; sample_theta += sample_delta)
    //{
    //    for (float sample_phi = 0.0f; sample_phi < HALF_PI; sample_phi += sample_delta)
    //    {
    //        float3 tangent_space_sample = normalize(float3(sin(sample_phi) * cos(sample_theta), cos(sample_phi), sin(sample_phi) * sin(sample_theta)));
    //        float3 world_space_normal = normalize(mul(tnb, tangent_space_sample));
            
    //        accum += cubemap.Sample(cubemapSampler, world_space_normal).rgb * cos(sample_phi) * sin(sample_phi);
    //        num_samples += 1.0f;
    //    }
    //}
    
    accum /= num_samples;
    output.col = /*PI * */accum;
        
    //float3 tangent_space_sample = up; //normalize(float3(sin(sample_phi) * cos(sample_theta), cos(sample_phi), sin(sample_phi) * sin(sample_theta)));
    //float3 world_space_normal = normalize(mul(tnb, tangent_space_sample));
            
    //output.col = cubemap.Sample(cubemapSampler, world_space_normal).rgb * cos(sample_phi) * sin(sample_phi);
    
    return output;
}