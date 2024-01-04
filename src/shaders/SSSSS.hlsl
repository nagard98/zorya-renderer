#define FIBONACCI_SEQUENCE_ANGLE(x) (((float(x) + 0.5)*(1 + sqrt(5))*0.5) * 2 * PI)

Texture2D diffuseIrrad : register(t0);
Texture2D specularIrrad : register(t1);
Texture2D transmittedIrrad : register(t2);
Texture2D depthMap : register(t3);

SamplerState texSampler : register(s0);

cbuffer matPrms : register(b1)
{
    float4 samples[16];
	float4 baseColor;
	float4 subsurfaceAlbedo;
	float4 meanFreePathColor;
	bool hasAlbedoMap;
	bool hasMetalnessMap;
	bool hasNormalMap;
	bool hasSmoothnessMap;
    
	float cb_roughness;
	float cb_metallic;
    
	float meanFreePathDist;
	float scale;
}

static const float PI = 3.14159265f;
static const float RCP_PI = 1 / PI;
static const float GAMMA = 1.0f / 2.2f;
static const float RCP_GAMMA = 1.0f / GAMMA;
static const float LOG2_E = log2(2.71828f);
static const float RCP_PI_8 = 1.0f / (3.14159f * 8.0f);

static const float FAR_PLANE = 100.0f;
static const float NEAR_PLANE = 1.0f;
static const float Q = FAR_PLANE / (FAR_PLANE - NEAR_PLANE);
static const float ALPHA = 11.0f;
static const float BETA = 800.0f;

static const float SCREEN_WIDTH = 1280.0f;
static const float SCREEN_HEIGHT = 720.0f;
static const float VERT_FOV = (PI / 2.0f) * (SCREEN_HEIGHT / SCREEN_WIDTH);
static const float ASPECT_RATIO = SCREEN_WIDTH / SCREEN_HEIGHT;
static const float NP_HEIGHT = 2.0f * NEAR_PLANE * tan(VERT_FOV / 2.0f);
static const float NP_WIDTH = NP_HEIGHT * ASPECT_RATIO;
static const float2 NP_SIZE = float2(NP_WIDTH, NP_HEIGHT);


static const float gaussWidths[6] = { 0.08f, 0.22f, 0.4324f, 0.7529f, 1.4106f, 2.722f };
static const float3 weights[6] = { 
    float3(0.233f, 0.455f, 0.649f),
    float3(0.100f, 0.336f, 0.344f),
    float3(0.118f, 0.196f, 0.0f),
    float3(0.113f, 0.007f, 0.007f),
    float3(0.358f, 0.004f, 0.0f),
    float3(0.078f, 0.0f, 0.0f),
};

float gauss(float r, float v)
{
    float twoV = 2.0f * v;
    return RCP_PI * (1.0f / twoV) * exp(-(r * r) / twoV);
}

float rand(float x)
{
    return frac(sin(x) * 100000.0f);
}

float pseudoRandom(float2 uv, float time)
{
    float value = sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453 + time;
    return frac(sin(value) * 143758.5453);
}

float3 computeBurleyDP(float r, float3 d)
{
    //rR(r)
    return (exp(-r / d) + exp(-r / (3.0f * d))) / (8.0f * PI * d);
}

float pdfBurleyDP(float r, float d)
{
	float pdf = (0.25f / d) * (exp(-r / d) + exp(-r / (3.0f * d)));
	return max(pdf, 0.00001f);
}

float4 f0(float4 y, float d, float4 r)
{
	return 1.0f - 0.25f * exp(-y / d) - 0.75f * exp(-y / (d * 3.0f)) - r;
}

float4 f1(float4 y, float d)
{
	return (0.25f / d) * (exp(-y / d) + exp(-y / (3.0f * d)));
}

float4 f2(float4 y, float d)
{
	return exp(-y / d) * (-0.0833333f * exp(2.0f * y / (3.0f * d)) - 0.25f) / (d * d);
}

float4 findZero(float4 y, float d, float4 r)
{
    float4 f0_y = f0(y, d, r);
    float4 y_k = y;
    float4 err = abs(f0_y);
    bool approxNotReached = err.x > 0.000001f && err.y > 0.000001f && err.z > 0.000001f && err.w > 0.000001f;
    while (approxNotReached)
    {
        float4 y_k1 = y_k - (f0(y_k, d, r)) / (f1(y_k, d) - 0.5f * ((f0(y_k, d, r) * f2(y_k, d)) / f1(y_k, d)));
        
        f0_y = f0(y_k1, d, r);
        y_k = y_k1;
    }
    
    return y_k;
}


float4 RadiusRootFindByApproximation(float D, float4 RandomNumber)
{
    return D * ((2.0f - 2.6f) * RandomNumber - 2.0f) * log(1.0f - RandomNumber);
}

float4 vs(uint vId : SV_VertexID) : SV_Position
{
    float2 texCoord = float2(vId & 1, vId >> 1);
    return float4((texCoord.x - 0.5f) * 2.0f, -(texCoord.y - 0.5f) * 2.0f, 0.0f, 1.0f);
}

float4 ps(float4 fragPos : SV_Position) : SV_Target
{
    float2 uvCoord = float2(fragPos.x / SCREEN_WIDTH, fragPos.y / SCREEN_HEIGHT);
       
    float centerDepth = (depthMap.Sample(texSampler, uvCoord));
    float centerDepthLin = (NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + centerDepth * (NEAR_PLANE - FAR_PLANE));
    
    float cellSize = 1.0f / 50.0f;
    float2 cellCoord = trunc(uvCoord * 50.0f) * cellSize;

    float sx = ALPHA / (centerDepthLin + BETA * abs(ddx(centerDepthLin)));
    float sy = ALPHA / (centerDepthLin + BETA * abs(ddy(centerDepthLin)));
    
    float3 col = float3(0.0f, 0.0f, 0.0f);
    
    float3 downsampledRad = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 3.0f).rgb, RCP_GAMMA);
    float3 rad = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA) + pow(transmittedIrrad.Sample(texSampler, uvCoord).rrr, RCP_GAMMA);

    float contrast = 0.0f;
    
    if (centerDepth < 0.99f)
    {
        contrast = max(pow(0.1f, 1.0f / GAMMA), (rad.r - downsampledRad.r) / downsampledRad.r);
    }
    
    float4 u = float4(0.0f,0.0f,0.0f,0.0f);
    float3 spec = float3(0.0f, 0.0f, 0.0f);
    
    if (centerDepth < 0.99f)
    {
        spec = pow(specularIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, 1.0f / GAMMA);

        float r;

        float3 dmfp = meanFreePathColor.rgb * meanFreePathDist;
		float maxDmfp = max(dmfp.r, max(dmfp.g, dmfp.b));
        
		float3 A = subsurfaceAlbedo.rgb;
		float maxA = max(A.r, max(A.g, A.b));
        
        float s = 3.5f + 100.0f * pow(maxA - 0.33, 4.0f);
        float3 s3 = 3.5f + 100.0f * pow(A - 0.33, 4.0f);
        
        float3 d3 = dmfp / s3;
		float d = maxDmfp / s;

        float3 sumWeights = float3(0.0f, 0.0f, 0.0f);
        float3 sss = float3(0.0f, 0.0f, 0.0f);
        
        float numSamples = 16.0f;
        for (float i = 0; i < numSamples; i+=1.0f)
        {
            u = samples[(int) i];
            float4 fourRadiuses =RadiusRootFindByApproximation(d, u);
            float radiuses[4];
            radiuses[0] = fourRadiuses.x;
            radiuses[1] = fourRadiuses.y;
            radiuses[2] = fourRadiuses.z;
            radiuses[3] = fourRadiuses.w;
            
            for (int j = 0; j < 4; j++)
            {
                float r = radiuses[j];
                
                float theta = FIBONACCI_SEQUENCE_ANGLE(i*4.0f+j);
                float cosTheta = cos(theta);
                float sinTheta = sin(theta);
            
                float2 dir = float2(cosTheta, sinTheta) * r; 
                dir = NEAR_PLANE * dir / centerDepthLin; //perspective division
                dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
                dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f) * scale; //raster coord system dir
                
                float2 sampleCoord = uvCoord + dir;
                float sampleDepth = depthMap.Sample(texSampler, sampleCoord);
                sampleDepth = (NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + sampleDepth * (NEAR_PLANE - FAR_PLANE));
                float deltaDepth = (centerDepthLin - sampleDepth) / 10.0f;
                float r1 = sqrt((deltaDepth * deltaDepth) + (r * r));
                
                dir = float2(cosTheta, sinTheta) * r1;
                dir = NEAR_PLANE * dir / centerDepthLin; //perspective division
                dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
                dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f) * scale; //raster coord system dir
                
                sampleCoord = uvCoord + dir;
                float3 rd = pow(diffuseIrrad.SampleLevel(texSampler, sampleCoord, 0.0f).rgb, RCP_GAMMA) + pow(transmittedIrrad.Sample(texSampler, sampleCoord).rgb, RCP_GAMMA);

			    float pdf = pdfBurleyDP(r, d);
                float3 diffusionProfile = computeBurleyDP(r1, d3);
                
                float3 sampleWeight = (diffusionProfile / pdf);
                sumWeights += sampleWeight;
            
                sss.xyz += sampleWeight * rd.rgb;
            }
		}
        
        sss /= sumWeights;
        col = sss;
        
        //contrast = max(pow(float3(0.0f, 0.0f, 0.0f), RCP_GAMMA), abs(rad.r - downsampledRad.r) / downsampledRad.r);
    }
    
    //contrast = ((clamp(contrast, 0.5f, 1.0f) - 0.5f) * 2) > 0.2f ? 1.0f : 0.0f;
    

    //if (true/*contrast > 0.0f*/)
    //{
    //    for (int j = 0; j < 6; j++)
    //    {
    //        float netFilterWidth = (1 / 1280.0f) * gaussWidths[j] * sx;
    //        float2 coords = uvCoord - float2(netFilterWidth * 3.0, 0.0);
        
    //        float variance = gaussWidths[j] * gaussWidths[j];
    //        float gaussian[7];
    //        gaussian[0] = gauss(netFilterWidth * -3.0f, variance);
    //        gaussian[1] = gauss(netFilterWidth * -2.0f, variance);
    //        gaussian[2] = gauss(netFilterWidth * -1.0f, variance);
    //        gaussian[3] = gauss(netFilterWidth * 0.0f, variance);
    //        gaussian[4] = gauss(netFilterWidth * 1.0f, variance);
    //        gaussian[5] = gauss(netFilterWidth * 2.0f, variance);
    //        gaussian[6] = gauss(netFilterWidth * 3.0f, variance);
        
    //        float totalSum = gaussian[0] + gaussian[1] + gaussian[2] + gaussian[3] + gaussian[4] + gaussian[5] + gaussian[6];
    //        for (int c = 0; c < 7; c++)
    //        {
    //            gaussian[c] /= totalSum;
    //        }
        
    //        for (int i = 0; i < 7; i++)
    //        {
    //            float3 rad = pow(diffuseIrrad.Sample(texSampler, coords).rgb, 1.0f / GAMMA) + pow(transmittedIrrad.Sample(texSampler, coords).rgb, 1.0f / GAMMA);
    //            col += (gaussian[i] * (rad * weights[j])) * 0.5f;
    //            coords += float2(netFilterWidth, 0.0f);
    //        }
        
    //        netFilterWidth = (1 / 720.0f) * gaussWidths[j] * sy;
    //        coords = uvCoord - float2(0.0f, netFilterWidth * 3.0);
    //        for (int i2 = 0; i2 < 7; i2++)
    //        {
    //            float3 rad = pow(diffuseIrrad.Sample(texSampler, coords).rgb, 1.0f / GAMMA) + pow(transmittedIrrad.Sample(texSampler, coords).rgb, 1.0f / GAMMA);
    //            col += (gaussian[i2] * rad * weights[j]) * 0.5f;
    //            coords += float2(0.0f, netFilterWidth);
    //        }
    //    }
    //}
    
    return float4(pow(col + spec, GAMMA), 1.0f);
    
    }