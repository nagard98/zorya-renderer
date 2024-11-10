#include "common/BRDF.hlsli"

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

cbuffer draw_constants : register(b0)
{
    float roughness;
    bool is_brdf_lut_pass;
}

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

float3 importance_sample_ggx(float2 uv, float3 normal, float roughness)
{
    float a = roughness * roughness;
	
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = normalize(cross(normal, tangent));
    
    float3x3 tnb = transpose(float3x3(tangent, normal, bitangent));
	
    float sample_theta = 2 * PI * uv.x;
    float cos_phi = sqrt((1.0 - uv.y) / (1.0 + (a * a - 1.0) * uv.y));
    float sin_phi = sqrt(1.0 - cos_phi * cos_phi);
    //float pdf = (1 / PI) * cos(sample_phi) * sin(sample_phi);
        
    float3 tangent_space_sample = normalize(float3(sin_phi * cos(sample_theta), cos_phi, sin_phi * sin(sample_theta)));
    float3 world_space_sample = normalize(mul(tnb, tangent_space_sample));
    
    return world_space_sample;
    
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float2 integrate_brdf(float NdotV, float roughness)
{
    float3 V = float3(sqrt(1.0 - NdotV * NdotV), 0.0f, NdotV);

    float A = 0.0f;
    float B = 0.0f;
    
    float3 N = float3(0.0f, 0.0f, 1.0f);
    
    float num_samples = 1024;

    for (uint i = 0; i < num_samples; i++)
    {
        float2 uv = Hammersley(i, num_samples);
        float3 H = importance_sample_ggx(uv, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        
        if (NdotL > 0.0f)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
                    
            A += (1.0 - Fc) * G_vis;
            B += Fc * G_vis;
        }
    }
    
    A /= num_samples;
    B /= num_samples;
    
    return float2(A, B);
    
}


PS_OUTPUT ps(PS_INPUT input)
{
    PS_OUTPUT output;
        
    if (!is_brdf_lut_pass)
    {
    
        float3 normal = normalize(input.texCoord);
        float3 R = normal;
        float3 V = R;
    
        float sample_delta = 0.015f;
        float num_samples = 16384;
        float3 accum = float3(0.0f, 0.0f, 0.0f);
        float total_weight = 0.0f;
        
        for (uint i = 0; i < num_samples; i++)
        {
            float2 uv = Hammersley(i, num_samples);
            float3 H = importance_sample_ggx(uv, normal, roughness);
            float3 L = normalize(2.0 * dot(V, H) * H - V);
        
            float NdotL = max(dot(normal, L), 0.0);
        
            accum += ToneMapACES(cubemap.Sample(cubemapSampler, L).rgb) * NdotL;
            total_weight += NdotL;
        }
    
        output.col = accum / total_weight;
    }
    else
    {
        float2 uv = input.pos.xy / 512.0f;
        
        float2 integrated_brdf = integrate_brdf(uv.x, uv.y);
        output.col = float3(integrated_brdf.x, integrated_brdf.y, 0.0f);
    }
        
    return output;
}