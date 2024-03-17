struct DirectionalLight
{
    float4 dir;
    float nearPlaneDist;
    float farPlaneDist;
    float2 pad;
};

struct PointLight
{
    float4 posWorldSpace;
    
    float constant;
    float lin;
    float quad;
    
    float nearPlaneDist;
    float farPlaneDist;
    
    float3 pad;
};

struct SpotLight
{
    float4 posWorldSpace;
    float4 direction;
    float cosCutoffAngle;
    
    float nearPlaneDist;
    float farPlaneDist;
    
    float pad;
};