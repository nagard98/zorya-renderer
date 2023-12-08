#define CUBEMAP_FACE_POSITIVE_X 0.0f
#define CUBEMAP_FACE_NEGATIVE_X 1.0f
#define CUBEMAP_FACE_POSITIVE_Y 2.0f
#define CUBEMAP_FACE_NEGATIVE_Y 3.0f
#define CUBEMAP_FACE_POSITIVE_Z 4.0f
#define CUBEMAP_FACE_NEGATIVE_Z 5.0f

struct PS_INPUT
{
    float4 fPos : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 fNormal : NORMAL;
    float4 posViewSpace : POSITION0;
    float4 posLightSpace : POSITION1;
    float4 posWorldSpace : POSITION2;
    float3x3 tbn : TBNMATRIX;
};

struct PS_OUTPUT
{
    float4 vCol : SV_TARGET;
};

struct DirectionalLight
{
    float4 dir;
};

struct PointLight
{
    float4 posWorldSpace;
    
    float constant;
    float lin;
    float quad;
};

struct SpotLight
{
    float4 posWorldSpace;
    float4 direction;
    float cosCutoffAngle;
};

cbuffer light : register(b0)
{
    DirectionalLight dirLight;
    
    int numPLights;
    PointLight pointLights[16];
    float4 posPointLightViewSpace[16];
    
    int numSpotLights;
    SpotLight spotLights[16];
    float4 posSpotLightViewSpace[16];
    float4 dirSpotLightViewSpace[16];
};

cbuffer matPrms : register(b1)
{
    float4 baseColor;
    bool hasAlbedoMap;
    bool hasMetalnessMap;
    bool hasNormalMap;
    bool hasSmoothnessMap;
    
    float cb_roughness;
    float cb_metallic;
}

cbuffer omniDirShadowMatrixes : register(b2)
{
    float4x4 lightViewMat[16 * 6];
    float4x4 lightProjMat;
    
    float4x4 spotLightViewMat[16];
    float4x4 spotLightProjMat[16];
}

#define MAX_SHININESS 64
#define SPECULAR_STRENGTH 0.3f
#define INNER_REFLECTANCE 1.0f

static const float revPi = 1 / 3.14159f;
static const float gamma = 1.0f / 2.2f;
static const float lightDec = -(1 / 16.0f);
static const float texScale = 1 / 1024.0f;

Texture2D ObjTexture : register(t0);
Texture2D NormalMap : register(t1);
Texture2D MetalnessMap : register(t2);
Texture2D SmoothnessMap : register(t3);
Texture2D ShadowMap : register(t4);
Texture2DArray ShadowCubeMap : register(t5);
//TextureCube ShadowCubeMap : register(t5);
Texture2D SpotShadowMap : register(t6);

SamplerState ObjSamplerState : register(s0);

float3 computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex, float metalness, float roughness);
float3 computeColPointLight(PointLight pLight, float4 posLightViewSpace, float4 fragPosViewSpace, float3 normal, float3 viewDir, float4 tex, float metalness, float roughness);
float3 computeColSpotLight(float4 posFragViewSpace, float3 normal, float metalness, float roughness);

float computeShadowing(float4 posLightSpace, float bias, float NdotL);
float computeSpotShadowing(float4 posWorldSpace, float4 posFragViewSpace, float minBias, float maxBias);
float computeOmniShadowing(float4 posWorldSpace, float4 posViewSpace, float3 normalViewSpace, float minBias, float maxBias);

float3 brdf(float VdotN, float LdotN, float LdotH, float NdotH, float linRoughness, float metalness);

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
    
    float metalness = cb_metallic;
    if (hasMetalnessMap == true)
    {
        metalness = MetalnessMap.Sample(ObjSamplerState, input.texCoord).r;
    }
    
    float roughness = cb_roughness;
    if (hasSmoothnessMap == true)
    {
        roughness = SmoothnessMap.Sample(ObjSamplerState, input.texCoord).g;
    }
    float linRoughness = pow(roughness, 4.0f);
    
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
    
    float shadowing = computeShadowing(input.posLightSpace, 0.01f, 0.0f);
    float3 radExitance = dirLight.dir.w == 0.0f ? computeColDirLight(dirLight, viewDir, input.fNormal, tex, metalness, linRoughness) * shadowing : float3(0.0f, 0.0f, 0.0f);
    
    shadowing = computeSpotShadowing(input.posWorldSpace, input.posViewSpace, 0.005f, 0.01f);
    radExitance += computeColSpotLight(input.posViewSpace, input.fNormal, metalness, linRoughness) * shadowing;
    
    for (int i = 0; i < numPLights; i++)
    {
        shadowing = computeOmniShadowing(input.posWorldSpace, input.posViewSpace, input.fNormal, 0.005f, 0.01f);
        radExitance += computeColPointLight(pointLights[i], posPointLightViewSpace[i], input.posViewSpace, input.fNormal, viewDir, tex, metalness, linRoughness) * shadowing;
    }
    
    float3 ambCol = 0.03f * tex.rgb;
    float3 col = radExitance * tex.rgb + ambCol;
    
    output.vCol = float4(saturate(pow(col, gamma)), 1.0f);
    
    return output;
}


float Fd_Lambertian()
{
    return revPi;
}

float Fd_Burley(float VdotN, float LdotN, float LdotH, float linRoughness)
{
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


float3 F_Schlick(float3 f0, float f90, float angle)
{
    return f0 + (f90 - f0) * pow(1.0f - angle, 5.0f);
}

float D_GGX(float NdotH, float roughness)
{
    float r2 = roughness * roughness;
    float f = (NdotH * r2 - NdotH) * NdotH + 1;
    return r2 / (f * f);
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    float alphaG2 = alphaG * alphaG;
    float lambdaV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float lambdaL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / (lambdaV + lambdaL);
}

float computeShadowing(float4 posLightSpace, float bias, float NdotL)
{
    float correctedBias = max(0.003f, bias * (1.0f - abs(NdotL)));
    float3 ndCoords = posLightSpace.xyz / posLightSpace.w;
    float currentDepth = ndCoords.z;
    float2 shadowMapUV = ndCoords.xy * 0.5f + 0.5f;
    shadowMapUV = float2(shadowMapUV.x, 1.0f - shadowMapUV.y);
    float sampledDepth = ShadowMap.Sample(ObjSamplerState, shadowMapUV).r;
    
    float shadowingSampled = 1.0f;
    //Brute force 16 samples
    for (float i = -1.5; i <= 1.5; i += 1.0f)
    {
        for (float j = -1.5; j <= 1.5; j += 1.0f)
        {
            float2 offset = float2(i,j) * texScale;
            float sampledDepth = ShadowMap.Sample(ObjSamplerState, shadowMapUV + offset).r;
            shadowingSampled += sampledDepth < currentDepth - correctedBias ? lightDec : 0.0f;
        }
    }
    
    return shadowingSampled;
}

float computeSpotShadowing(float4 posWorldSpace, float4 posFragViewSpace, float minBias, float maxBias)
{
    float shadowingSampled = 1.0f;
    float allLightShadowing = 0.0f;
    float correctedBias = 0.004f; // max(0.001f, bias * (1.0f - abs(NdotL)));
    
    for (int i = 0; i < numSpotLights; i++)
    {
        shadowingSampled = 1.0f;
        
        float3 lightDir = normalize(posSpotLightViewSpace[i] - posFragViewSpace);
        float cosAngle = dot(-lightDir, normalize(dirSpotLightViewSpace[i].xyz));
        if (cosAngle > spotLights[i].cosCutoffAngle)
        {
            float4 posLightSpace = mul(posWorldSpace, mul(spotLightViewMat[i], spotLightProjMat[i]));
            float3 ndcPosLightSpace = (posLightSpace.xyz / posLightSpace.w);
        
            if (ndcPosLightSpace.x < 1.0f && ndcPosLightSpace.y < 1.0f && ndcPosLightSpace.z < 1.0f &&
            ndcPosLightSpace.x > -1.0f && ndcPosLightSpace.y > -1.0f && ndcPosLightSpace.z > 0.0f)
            {
                //float3 sampleDir = posWorldSpace - float4(pointLights[0].posWorldSpace.x, 0.0f, 0.0f, 1.0f);
                float2 normCoords = (ndcPosLightSpace.xy * 0.5f) + 0.5f;
                
                //DirectX maps the near plane to 0, not -1
                float currentDepth = ndcPosLightSpace.z;
                float2 shadowMapUVF = float2(normCoords.x, 1.0f - normCoords.y);
    
                //Brute force 16 samples
                for (float i = -1.5; i <= 1.5; i += 1.0f)
                {
                    for (float j = -1.5; j <= 1.5; j += 1.0f)
                    {
                        float2 offset = float2(i, j) * texScale;
                        float sampledDepth = SpotShadowMap.Sample(ObjSamplerState, shadowMapUVF + offset).r;
                        shadowingSampled += sampledDepth < currentDepth - correctedBias ? lightDec : 0.0f;
                    }
                }
            }
        }
        
        allLightShadowing += shadowingSampled;
    }
    
    return allLightShadowing / numSpotLights;
}

float computeOmniShadowing(float4 posWorldSpace, float4 posViewSpace, float3 normalViewSpace, float minBias, float maxBias)
{
    float shadowingSampled = 1.0f;
    float allLightShadowing = 0.0f;
    
    for (int lightIndex = 0; lightIndex < numPLights; lightIndex++)
    {
        float3 lightDir = normalize(posPointLightViewSpace[lightIndex] - posViewSpace);
        float LdotN = saturate(dot(lightDir, normalViewSpace));
        float correctedBias = max(minBias, maxBias * (1.0f - LdotN));
        
        shadowingSampled = 1.0f;
        int cubemapOffset = lightIndex * 6;
        
        for (float face = 0; face < 6; face += 1.0f)
        {
            float4 posLightSpace = mul(posWorldSpace, mul(lightViewMat[face + cubemapOffset], lightProjMat));
            float3 ndcPosLightSpace = (posLightSpace.xyz / posLightSpace.w);
        
            if (ndcPosLightSpace.x < 1.0f && ndcPosLightSpace.y < 1.0f && ndcPosLightSpace.z < 1.0f &&
            ndcPosLightSpace.x > -1.0f && ndcPosLightSpace.y > -1.0f && ndcPosLightSpace.z > 0.0f)
            {
                //float3 sampleDir = posWorldSpace - float4(pointLights[0].posWorldSpace.x, 0.0f, 0.0f, 1.0f);
                float2 normCoords = (ndcPosLightSpace.xy * 0.5f) + 0.5f;
                
                //DirectX maps the near plane to 0, not -1
                float currentDepth = ndcPosLightSpace.z;
                float3 shadowMapUVF = float3(normCoords.x, 1.0f - normCoords.y, face + cubemapOffset);
    
                //Brute force 16 samples
                for (float i = -1.5; i <= 1.5; i += 1.0f)
                {
                    for (float j = -1.5; j <= 1.5; j += 1.0f)
                    {
                        float3 offset = float3(i, j, 0.0f) * texScale;
                        float sampledDepth = ShadowCubeMap.Sample(ObjSamplerState, shadowMapUVF + offset).r;
                        shadowingSampled += sampledDepth < currentDepth - correctedBias ? lightDec : 0.0f;
                    }
                }
                
            }
        }
        allLightShadowing += shadowingSampled;
    }
    
    return allLightShadowing / numPLights;
}

float3 computeColDirLight(DirectionalLight dLight, float3 viewDir, float3 normal, float4 tex, float metalness, float linRoughness)
{
    float3 lDir = -normalize(dLight.dir.xyz);
    //float3 lRef = reflect(lDir, normal);
    
    float3 halfVec = normalize(lDir + viewDir);

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    return brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness) * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi;
}

float3 computeColPointLight(PointLight pLight, float4 lightPosViewSpace, float4 fragPosViewSpace, float3 normal, float3 viewDir, float4 tex, float metalness, float linRoughness)
{
    float3 lDir = lightPosViewSpace - fragPosViewSpace;
    float dist = length(lDir);
    lDir = normalize(lDir);
    float3 halfVec = normalize(-normalize(fragPosViewSpace.xyz) + lDir.xyz);

    float attenuation = 1 / (pLight.constant + (pLight.lin * dist) + (pLight.quad * pow(dist, 2)));

    float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
    float LdotN = saturate(dot(lDir, normal));
    float LdotH = saturate(dot(lDir, halfVec));
    float NdotH = saturate(dot(halfVec, normal));
    
    return brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness) * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi * attenuation;
}

float3 computeColSpotLight(float4 posFragViewSpace, float3 normal, float metalness, float linRoughness)
{
    float3 radiantExitance = float3(0.0f, 0.0f, 0.0f);
        
    for (int i = 0; i < numSpotLights; i++)
    {
        float3 lightDir = normalize(posSpotLightViewSpace[i] - posFragViewSpace);
        float cosAngle = dot(-lightDir, normalize(dirSpotLightViewSpace[i].xyz));
        if (cosAngle > spotLights[i].cosCutoffAngle)
        {
            float3 viewDir = -normalize(posFragViewSpace).xyz;
            float3 halfVec = normalize(viewDir + lightDir);
            
            float VdotN = abs(dot(normal, viewDir)) + 1e-5f;
            float LdotN = saturate(dot(lightDir, normal));
            float LdotH = saturate(dot(lightDir, halfVec));
            float NdotH = saturate(dot(halfVec, normal));
    
            radiantExitance = brdf(VdotN, LdotN, LdotH, NdotH, linRoughness, metalness) * LdotN * float3(3.14f, 3.14f, 3.14f) * revPi;
        }
    }
    
    return radiantExitance;

}


float3 brdf(float VdotN, float LdotN, float LdotH, float NdotH, float linRoughness, float metalness)
{
    //float3 diffuse = saturate(INNER_REFLECTANCE * tex.rgb * Fd_Lambertian());
    float3 diffuse = Fd_Burley(VdotN, LdotN, LdotH, linRoughness);

    //float3 specular = (1.0f - INNER_REFLECTANCE) * Fs_BlinnPhong(roughness * MAX_SHININESS, NdotH) * SPECULAR_STRENGTH;
    float reflectance = 0.35f;
    float3 f0 = 0.16f * reflectance * reflectance * (1.0f - metalness) + metalness;

    float D = D_GGX(NdotH, linRoughness);
    float3 F = F_Schlick(f0, 1.0f, LdotH);
    float V = saturate(V_SmithGGXCorrelated(LdotN, VdotN, linRoughness));

    float3 specular = D * F * V;
    
    return (diffuse + specular);
}