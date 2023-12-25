Texture2D diffuseIrrad : register(t0);
Texture2D specularIrrad : register(t1);
Texture2D transmittedIrrad : register(t2);
Texture2D depthMap : register(t3);

SamplerState texSampler : register(s0);

static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f / 2.2f;

static const float zf = 100.0f;
static const float zn = 1.0f;
static const float q = zf / (zf - zn);
static const float alpha = 11.0f;
static const float beta = 800.0f;

static const float gaussWidths[6] = { 0.08f, 0.22f, 0.4324f, 0.7529f, 1.4106f, 2.722f };
static const float3 weights[6] = { 
    float3(0.233f, 0.455f, 0.649f),
    float3(0.100f, 0.336f, 0.344f),
    float3(0.118f, 0.196f, 0.0f),
    float3(0.113f, 0.007f, 0.007f),
    float3(0.358f, 0.004f, 0.0f),
    float3(0.078f, 0.0f, 0.0f),
};


float4 vs(uint vId : SV_VertexID) : SV_Position
{
    float2 texCoord = float2(vId & 1, vId >> 1);
    return float4((texCoord.x - 0.5f) * 2.0f, -(texCoord.y - 0.5f) * 2.0f, 0.0f, 1.0f);
}

float gauss(float r, float v)
{
    float twoV = 2.0f * v;
    return revPi * (1.0f / twoV) * exp(-(r * r) / twoV);
}

float4 ps(float4 fragPos : SV_Position) : SV_Target
{
    float2 uvCoord = float2(fragPos.x / 1280.0f, fragPos.y / 720.0f);
    
    float depth = (zn * zf) / (zf + (depthMap.Sample(texSampler, uvCoord)) * (zn - zf));
    //float depth = depthMap.Sample(texSampler, uvCoord);
    //depth = (-zn * q) / (depth - q);
    float sx = alpha / (depth + beta * abs(ddx(depth)));
    float sy = alpha / (depth + beta * abs(ddy(depth)));
    
    float3 col = float3(0.0f, 0.0f, 0.0f);
    
    for (int j = 0; j < 6; j++)
    {
        float netFilterWidth = (1 / 1280.0f) * gaussWidths[j] * sx;
        float2 coords = uvCoord - float2(netFilterWidth * 3.0, 0.0);
        
        float variance = gaussWidths[j] * gaussWidths[j];
        float gaussian[7];
        gaussian[0] = gauss(netFilterWidth * -3.0f, variance);
        gaussian[1] = gauss(netFilterWidth * -2.0f, variance);
        gaussian[2] = gauss(netFilterWidth * -1.0f, variance);
        gaussian[3] = gauss(netFilterWidth * 0.0f, variance);
        gaussian[4] = gauss(netFilterWidth * 1.0f, variance);
        gaussian[5] = gauss(netFilterWidth * 2.0f, variance);
        gaussian[6] = gauss(netFilterWidth * 3.0f, variance);
        
        float totalSum = gaussian[0] + gaussian[1] + gaussian[2] + gaussian[3] + gaussian[4] + gaussian[5] + gaussian[6];
        for (int c = 0; c < 7; c++)
        {
            gaussian[c] /= totalSum;
        }
        
        for (int i = 0; i < 7; i++)
        {
            float3 rad = pow(diffuseIrrad.Sample(texSampler, coords).rgb, 1.0f / gamma) + pow(transmittedIrrad.Sample(texSampler, coords).rgb, 1.0f / gamma);
            col += (gaussian[i] * (rad * weights[j])) * 0.5f;
            coords += float2(netFilterWidth, 0.0f);
        }
        
        netFilterWidth = (1 / 720.0f) * gaussWidths[j] * sy;
        coords = uvCoord - float2(0.0f, netFilterWidth * 3.0);
        for (int i2 = 0; i2 < 7; i2++)
        {
            float3 rad = pow(diffuseIrrad.Sample(texSampler, coords).rgb, 1.0f / gamma) + pow(transmittedIrrad.Sample(texSampler, coords).rgb, 1.0f / gamma);
            col += (gaussian[i2] * rad * weights[j]) * 0.5f;
            coords += float2(0.0f, netFilterWidth);
        }
    }
    
    col = saturate(pow(specularIrrad.Sample(texSampler, uvCoord.xy).rgb, 1.0f/gamma) + col);
    
    return float4(pow(col, gamma), 1.0f);

}