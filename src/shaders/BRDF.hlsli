struct brdfOut_t
{
    float3 diffuse;
    float3 specular;
};

brdfOut_t brdf(float VdotN, float LdotN, float LdotH, float NdotH, float linRoughness, float metalness);

float Fd_Lambertian();
float Fd_Burley(float VdotN, float LdotN, float LdotH, float linRoughness);

float Fs_BlinnPhong(float shininess, float HdotN);
float Fs_CookTorrance(float distr, float visib, float fresnel);

float3 F_Schlick(float3 f0, float f90, float angle);
float D_GGX(float NdotH, float roughness);
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG);


brdfOut_t brdf(float VdotN, float LdotN, float LdotH, float NdotH, float linRoughness, float metalness)
{
    //float3 diffuse = saturate(INNER_REFLECTANCE * tex.rgb * Fd_Lambertian());
    brdfOut_t brdfOut;
    brdfOut.diffuse = Fd_Burley(VdotN, LdotN, LdotH, linRoughness);

    //float3 specular = (1.0f - INNER_REFLECTANCE) * Fs_BlinnPhong(roughness * MAX_SHININESS, NdotH) * SPECULAR_STRENGTH;
    float reflectance = 0.35f;
    float3 f0 = 0.16f * reflectance * reflectance * (1.0f - metalness) + metalness;

    float D = D_GGX(NdotH, linRoughness);
    float3 F = F_Schlick(f0, 1.0f, LdotH);
    float V = saturate(V_SmithGGXCorrelated(LdotN, VdotN, linRoughness));

    brdfOut.specular = D * F * V;
    
    return brdfOut;
}

float Fd_Lambertian()
{
    return 1/3.14159f;
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