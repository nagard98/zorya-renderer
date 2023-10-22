struct PS_INPUT {
	float4 fPos		: SV_POSITION;
    float2 texCoord : TEXCOORD0;
	float3 fNormal : NORMAL0;
    float4 posViewSpace : POSITION0;
};

struct PS_OUTPUT {
	float4 vCol		: SV_TARGET;
};

cbuffer light : register(b0)
{
    float4 lightDir;
};

Texture2D ObjTexture : register(t0);
SamplerState ObjSamplerState : register(s0);

PS_OUTPUT ps(PS_INPUT input) {
	PS_OUTPUT output;

    input.fNormal = normalize(input.fNormal);
	
    float3 lDir = -normalize(lightDir.xyz);
    float3 lRef = reflect(lDir, input.fNormal);
    
    float4 tex = ObjTexture.Sample(ObjSamplerState, input.texCoord);
    
    float3 viewDir = -normalize(input.posViewSpace).xyz;
    float3 halfVec = normalize(lDir + viewDir);
    
    float3 col = float3(0.1f, 0.1f, 0.1f) * tex.xyz; //ambient

    float specular = pow(dot(halfVec, input.fNormal), 64);
    col += specular * 0.4 * float3(1.0f, 1.0f, 1.0f);
    
    col += saturate(tex * dot(input.fNormal, lDir));
	
    output.vCol = float4(col,tex.w);
    
	return output;
}