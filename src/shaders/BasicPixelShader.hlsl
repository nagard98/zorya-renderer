struct PS_INPUT {
	float4 fPos		: SV_POSITION;
    float2 texCoord : TEXCOORD0;
	float3 fNormal : NORMAL0;
    float4 posViewSpace : POSITION0;
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
    PointLight pointLights[1];
};

Texture2D ObjTexture : register(t0);
SamplerState ObjSamplerState : register(s0);

float3 computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex);
float3 computeColPointLight(PointLight pLight, float4 fragPos, float3 normal, float4 tex);

PS_OUTPUT ps(PS_INPUT input) 
{
	PS_OUTPUT output;

    input.fNormal = normalize(input.fNormal);
    
    float4 tex = ObjTexture.Sample(ObjSamplerState, input.texCoord);
    
    float3 viewDir = -normalize(input.posViewSpace).xyz;
    
    //float3 col = float3(0.1f, 0.1f, 0.1f) * tex.xyz; //ambient
    float3 col = float3(0.0f, 0.0f, 0.0f);
    col += computeColDirLight(dirLight, viewDir, input.fNormal, tex);
    
    for (int i = 0; i < 1; i++)
    {
        col += computeColPointLight(pointLights[i], input.posViewSpace, input.fNormal, tex);
    }

    output.vCol = float4(saturate(col), tex.w);
    
	return output;
}

float3 computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex)
{
    float3 lDir = -normalize(dLight.dir.xyz);
    //float3 lRef = reflect(lDir, normal);
    
    float3 halfVec = normalize(lDir + viewDir);
    
    float3 diffuseCol = saturate(tex * dot(normal, lDir));
    float3 specularCol = pow(dot(halfVec, normal), 8) * 0.3f * float3(1.0f, 1.0f, 1.0f);
    float3 ambCol = float3(0.2f, 0.2f, 0.2f) * tex.xyz;
    
    return saturate(diffuseCol + specularCol + ambCol);
}

float3 computeColPointLight(PointLight pLight, float4 fragPos, float3 normal, float4 tex)
{
    float4 lightDir = pLight.pos - fragPos;
    float dist = length(lightDir);
    lightDir = normalize(lightDir);
    float attenuation = 1 / (pLight.constant + (pLight.lin * dist) + (pLight.quad * pow(dist, 2)));
    
    float3 halfVec = normalize(-normalize(fragPos.xyz) + lightDir);
    
    float3 diffCol = saturate(attenuation * tex * dot(lightDir, normal));
    float3 specCol = attenuation * pow(dot(halfVec, normal), 16) * 0.3f * float3(1.0f,1.0f,1.0f);
    float3 ambCol = attenuation * float3(0.3f, 0.3f, 0.3f) * tex.xyz;
    
    return saturate(ambCol + diffCol + specCol);
}
