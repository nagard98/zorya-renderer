static const float SCREEN_WIDTH = 1920.0f;
static const float SCREEN_HEIGHT = 1080.0f;

Texture2D lighted_scene : register(t0);

SamplerState tex_sampler : register(s0);

float4 ps(float4 fragPos : SV_Position) : SV_Target0
{
    float2 uvCoord = float2(fragPos.x / SCREEN_WIDTH, fragPos.y / SCREEN_HEIGHT);
    
    return lighted_scene.Sample(tex_sampler, uvCoord);
}