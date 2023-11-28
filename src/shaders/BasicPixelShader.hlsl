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
    
    float roughness;
    float metallic;
}

#define MAX_SHININESS 64
#define SPECULAR_STRENGTH 0.3f
#define INNER_REFLECTANCE 1.0f

static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f/2.2f;

Texture2D ObjTexture : register(t0);
Texture2D NormalMap : register(t1);
SamplerState ObjSamplerState : register(s0);

float3 computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex);
float3 computeColPointLight(PointLight pLight, float4 fragPos, float3 normal, float3 viewDir, float4 tex);

float Fd_Lambertian();
float Fd_Burley(float VdotN, float LdotN, float LdotH, float linRoughness);

float Fs_BlinnPhong(float shininess, float HdotN);
float Fs_CookTorrance(float distr, float visib, float fresnel);

float3 F_Schlick(float3 f0, float f90, float angle);
float D_GGX(float NdotH, float roughness);
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG);

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
        tex = ObjTexture.Sample(ObjSamplerState, input.texCoord) * pow(baseColor, 1.0f / gamma);
    }
    else
    {
        tex = pow(baseColor, 1.0f / gamma);
    }
    
    float3 viewDir = -normalize(input.posViewSpace).xyz;
    
    float3 col = dirLight.dir.w == 0.0f ? computeColDirLight(dirLight, viewDir, input.fNormal, tex) : float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < numPLights; i++)
    {
        col += computeColPointLight(pointLights[i], input.posViewSpace, input.fNormal, viewDir, tex);
    }

    output.vCol = float4(saturate(pow(col, gamma)), 1.0f);
    
	return output;
}

float Fd_Lambertian()
{
    return revPi;
}

float Fd_Burley(float VdotN, float LdotN, float LdotH, float linRoughness) {
    float energyBias = lerp(0.0f, 0.5f, linRoughness);
    float energyFactor = lerp(1.0f, 1.0f / 1.51f, linRoughness);

    float fd90 = energyBias + 2.0f * LdotH * LdotH * linRoughness;
    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    float viewSchlick = F_Schlick(f0, fd90, VdotN).r;
    float lightSchlick = F_Schlick(f0, fd90, LdotN).r;

    return energyFactor * viewSchlick * lightSchlick;
}


float Fs_BlinnPhong(float shininess, float HdotN)
{
    return pow(HdotN, max(shininess, 1.0f));
}


float3 F_Schlick(float3 f0, float f90, float angle) {
    return f0 + (f90 - f0) * pow(1.0f - angle, 5.0f);
}

float D_GGX(float NdotH, float roughness) {
    float r2 = roughness * roughness;
    float f = (NdotH * r2 - NdotH) * NdotH + 1;
    return r2 / (f * f);
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG) {
    float alphaG2 = alphaG * alphaG;
    float lambdaV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float lambdaL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / (lambdaV + lambdaL);
}

float3 computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex)
{
    float3 lDir = -normalize(dLight.dir.xyz);
    //float3 lRef = reflect(lDir, normal);
    
    float3 halfVec = normalize(lDir + viewDir);

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));

    float linRoughness = pow(roughness, 4.0f);
    
    //float3 diffuse = tex.rgb * float3(3.14f, 3.14f, 3.14f) * Fd_Lambertian();
    float3 diffuse = Fd_Burley(VdotN, LdotN, LdotH, linRoughness) * tex.rgb;

    //float3 specular = float3(1.0f, 1.0f, 1.0f) * Fs_BlinnPhong((1.0f - roughness) * MAX_SHININESS, specAngle) * SPECULAR_STRENGTH;
    float reflectance = 0.35f;
    float3 f0 = 0.16f * reflectance * reflectance * (1.0f - metallic) + tex.rgb * metallic;

    float D = D_GGX(NdotH, linRoughness);
    float3 F = F_Schlick(f0, 1.0f, LdotH);
    float V = saturate(V_SmithGGXCorrelated(LdotN, VdotN, linRoughness));

    float3 specular = D * F * V;
    
    float3 ambCol = float3(0.05f, 0.05f, 0.05f) * tex.rgb;
    
    return saturate(ambCol + (diffuse + specular) * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi);
}

float3 computeColPointLight(PointLight pLight, float4 fragPos, float3 normal, float3 viewDir, float4 tex)
{
    float4 lDir = pLight.pos - fragPos;
    float dist = length(lDir);
    lDir = normalize(lDir);
    float3 halfVec = normalize(-normalize(fragPos.xyz) + lDir.xyz);

    float attenuation = 1 / (pLight.constant + (pLight.lin * dist) + (pLight.quad * pow(dist, 2)));
    float linRoughness = pow(roughness, 4.0f);

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    //float3 diffuse = saturate(INNER_REFLECTANCE * tex.rgb * Fd_Lambertian());
    float3 diffuse = Fd_Burley(VdotN, LdotN, LdotH, linRoughness) * tex.rgb;

    //float3 specular = (1.0f - INNER_REFLECTANCE) * Fs_BlinnPhong(roughness * MAX_SHININESS, NdotH) * SPECULAR_STRENGTH;
    float reflectance = 0.35f;
    float3 f0 = 0.16f * reflectance * reflectance * (1.0f - metallic) + tex.rgb * metallic;

    float D = D_GGX(NdotH, linRoughness);
    float3 F = F_Schlick(f0, 1.0f, LdotH);
    float V = saturate(V_SmithGGXCorrelated(LdotN, VdotN, linRoughness));

    float3 specular = D * F * V;

    float3 ambCol = attenuation * float3(0.05f, 0.05f, 0.05f) * tex.xyz;
    
    return saturate(ambCol + (diffuse + specular) * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation);
}
