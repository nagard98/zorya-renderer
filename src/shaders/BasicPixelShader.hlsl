struct PS_INPUT {
	float4 vPos		: SV_POSITION;
    float2 texCoord : TEXCOORD0;
	//float4 vCol		: COLOR;
};

struct PS_OUTPUT {
	float4 vCol		: SV_TARGET;
};

Texture2D ObjTexture : register(t0);
SamplerState ObjSamplerState : register(s0);

PS_OUTPUT ps(PS_INPUT input) {
	PS_OUTPUT output;

    output.vCol = ObjTexture.Sample(ObjSamplerState, input.texCoord);

	return output;
}