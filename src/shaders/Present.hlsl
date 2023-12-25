Texture2D finalOutput : register(t0);
SamplerState texSampler : register(s0);

static const float gamma = 1.0f / 2.2f;

float4 ps(float4 fragPos : SV_Position) : SV_Target
{
    float2 uvCoord = float2(fragPos.x / 1280.0f, fragPos.y / 720.0f);
    return saturate(pow(finalOutput.Sample(texSampler, uvCoord), 1.0f));
}