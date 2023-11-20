struct PS_INPUT {
	float4 fPos		: SV_POSITION;
    float2 texCoord : TEXCOORD0;
	float3 fNormal : NORMAL;
    float4 posViewSpace : POSITION0;
    float3x3 tbn : TBNMATRIX;
};

struct PS_OUTPUT {
	float4 vCol		: SV_TARGET;
};

struct DirectionalLight
{
    float4 dir;
};

struct PointLight
{
    float4 pos;
    
    float constant;
    float lin;
    float quad;
};

cbuffer light : register(b0)
{
    DirectionalLight dirLight;
    
    int numPLights;
    PointLight pointLights[16];
    
};

cbuffer matPrms : register(b1)
{
    float4 baseColor;
    bool hasAlbedoMap;
    bool hasMetalnessMap;
    bool hasNormalMap;
    
    float smoothness;
    float metalness;
}

#define MAX_SHININESS 64
#define SPECULAR_STRENGTH 0.3f
#define INNER_REFLECTANCE 0.9f

static const float revPi = 1 / 3.14159f;

Texture2D ObjTexture : register(t0);
Texture2D NormalMap : register(t1);
SamplerState ObjSamplerState : register(s0);

float3 computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex);
float3 computeColPointLight(PointLight pLight, float4 fragPos, float3 normal, float4 tex);

float3 lambertianDiffuse(float3 baseColor, float3 lightColor, float NdotL);

PS_OUTPUT ps(PS_INPUT input) 
{
	PS_OUTPUT output;

    if (hasNormalMap == true)
    {
        input.fNormal = normalize(NormalMap.Sample(ObjSamplerState, input.texCoord).rgb * 2.0f - 1.0f);
        input.fNormal = mul(input.tbn, input.fNormal);
    }
    else
    {
        input.fNormal = normalize(input.fNormal);
    }
    
    float4 tex = 0.0f;
    if (hasAlbedoMap == true)
    {
        tex = ObjTexture.Sample(ObjSamplerState, input.texCoord) * baseColor;

    }
    else
    {
        tex = baseColor;
    }
    
    
    float3 viewDir = -normalize(input.posViewSpace).xyz;
    
    float3 col = dirLight.dir.w == 0.0f ? computeColDirLight(dirLight, viewDir, input.fNormal, tex) : float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < numPLights; i++)
    {
        col += computeColPointLight(pointLights[i], input.posViewSpace, input.fNormal, tex);
    }

    float gamma = 1/2.2f;
    output.vCol = float4(saturate(pow(col, gamma)), tex.w);
    
	return output;
}

float3 lambertianDiffuse(float3 baseColor, float3 lightColor, float NdotL)
{
    return saturate(baseColor * revPi * lightColor * NdotL);
}

float3 blinnPhongSpecular(float3 lightColor, float shininess, float HdotN)
{
    return saturate(pow(HdotN, max(shininess, 1.0f)) * lightColor);
}

float3 computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex)
{
    float3 lDir = -normalize(dLight.dir.xyz);
    //float3 lRef = reflect(lDir, normal);
    
    float3 halfVec = normalize(lDir + viewDir);
    
    float3 diffuseCol = INNER_REFLECTANCE * lambertianDiffuse(tex.rgb, float3(3.14f, 3.14f, 3.14f), dot(lDir, normal));
    
    float specAngle = max(dot(halfVec, normal), 0.0f);
    float3 specularCol = (1.0f - INNER_REFLECTANCE) * blinnPhongSpecular(1.0f, smoothness * MAX_SHININESS, specAngle) * SPECULAR_STRENGTH;
    
    float3 ambCol = float3(0.05f, 0.05f, 0.05f) * tex.rgb;
    
    return saturate(diffuseCol + specularCol + ambCol);
}

float3 computeColPointLight(PointLight pLight, float4 fragPos, float3 normal, float4 tex)
{
    float4 lightDir = pLight.pos - fragPos;
    float dist = length(lightDir);
    lightDir = normalize(lightDir);
    float attenuation = 1 / (pLight.constant + (pLight.lin * dist) + (pLight.quad * pow(dist, 2)));
    
    float3 halfVec = normalize(-normalize(fragPos.xyz) + lightDir.xyz);
    
    float3 diffCol = INNER_REFLECTANCE * lambertianDiffuse(tex.rgb, float3(3.14f, 3.14f, 3.14f) * attenuation, dot(lightDir.xyz, normal));
    
    float specAngle = max(dot(halfVec, normal), 0.0f);
    float3 specCol = (1.0f - INNER_REFLECTANCE) * blinnPhongSpecular(1.0f * attenuation, smoothness * MAX_SHININESS, specAngle) * SPECULAR_STRENGTH;
    
    float3 ambCol = attenuation * float3(0.05f, 0.05f, 0.05f) * tex.xyz;
    
    return saturate(diffCol + specCol + ambCol);
}
