struct PS_INPUT
{
    float4 posLightProj : SV_POSITION;
};

struct PS_OUTPUT
{
    float distance : SV_Target;
};

PS_OUTPUT ps(PS_INPUT input)
{
    PS_OUTPUT output;
    
    return output;
}