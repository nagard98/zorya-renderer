#include "common/LightStruct.hlsli"

#define FIBONACCI_SEQUENCE_ANGLE(x) (((float(x) + 0.5)*(1 + sqrt(5))*0.5) * 2 * PI)
#define G_A 2.399963
#define HPI 1.570796

static const int numSamplesJimenezSep = 15;

Texture2D diffuseIrrad : register(t0);
Texture2D specularIrrad : register(t1);
Texture2D transmittedIrrad : register(t2);
Texture2D depthMap : register(t3);
Texture2D gbuffer_albedo : register(t4);

SamplerState texSampler : register(s0);

struct PS_OUT
{
    float4 finalComposited : SV_Target0;
    float4 lastBiggestBlur : SV_Target1;
};

cbuffer light : register(b0)
{
    //scene lights
    Directional_Light dirLight;
    
    int numPLights;
    int numSpotLights;
    int2 pad;
    
    Point_Light pointLights[16];
    float4 posPointLightViewSpace[16];
    
    Spot_Light spotLights[16];
    float4 posSpotLightViewSpace[16];
    float4 dirSpotLightViewSpace[16];
    
    //int sssModelId;
    //int pad3[3];
    
    //golubev subsurface scattering model
    float4 samples[16];
    float4 subsurfaceAlbedo;
    float4 meanFreePathColor;
    
    float meanFreePathDist;
    float scale;
    
    float2 dir;
    int numSamples;
    int3 pad2;

    //golubev subsurface scattering model
    float4 kernel[numSamplesJimenezSep];
    //float4 kernel[16];
};


cbuffer matPrms : register(b1)
{
    //float4 samples[16];
	float4 baseColor;
	//float4 subsurfaceAlbedo;
	//float4 meanFreePathColor;
	bool hasAlbedoMap;
	bool hasMetalnessMap;
	bool hasNormalMap;
	bool hasSmoothnessMap;
    
	float cb_roughness;
    float cb_metallic;
    
	//float meanFreePathDist;
	//float scale;
    
    //float4 kernel[16];
    //float2 dir;
    int sssModelId;
    int pad4;
}

cbuffer camMat : register(b5)
{
    float4x4 invCamProjMat;
    float4x4 invCamViewMat;
}

static const float PI = 3.14159265f;
static const float RCP_PI = 1 / PI;
static const float GAMMA = 1.0f / 2.2f;
static const float RCP_GAMMA = 1.0f / GAMMA;
static const float LOG2_E = log2(2.71828f);
static const float RCP_PI_8 = 1.0f / (3.14159f * 8.0f);

static const float FAR_PLANE = 10.0f;
static const float NEAR_PLANE = 0.01f;
static const float Q = FAR_PLANE / (FAR_PLANE - NEAR_PLANE);
static const float ALPHA = 3.0f;
static const float BETA = 800.0f;


static const float SCREEN_WIDTH = 1280.0f;
static const float SCREEN_HEIGHT = 720.0f;
static const float VERT_FOV = (PI / 2.0f) * (SCREEN_HEIGHT / SCREEN_WIDTH);
static const float ASPECT_RATIO = SCREEN_WIDTH / SCREEN_HEIGHT;
static const float NP_HEIGHT = 2.0f * NEAR_PLANE * tan(VERT_FOV / 2.0f);
static const float NP_WIDTH = NP_HEIGHT * ASPECT_RATIO;
static const float2 NP_SIZE = float2(NP_WIDTH, NP_HEIGHT);

static const float GOLDEN_RATIO = 1.6180339887f;
static const float GOLDEN_ANGLE = 2.399963f;

static const float gaussWidths[6] = { 0.08f, 0.22f, 0.4324f, 0.7529f, 1.4106f, 2.722f };
static const float3 weights[6] = { 
    float3(0.233f, 0.455f, 0.649f),
    float3(0.100f, 0.336f, 0.344f),
    float3(0.118f, 0.196f, 0.0f),
    float3(0.113f, 0.007f, 0.007f),
    float3(0.358f, 0.004f, 0.0f),
    float3(0.078f, 0.0f, 0.0f),
};

float4 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat)
{
    float x = quadTexCoord.x * 2.0f - 1.0f;
    float y = (1.0f - quadTexCoord.y) * 2.0f - 1.0f;
    
    float4 vProjPos = float4(x, y, sampledDepth, 1.0f);
    float4 vPosVS = mul(vProjPos, invMat);
    
    return vPosVS.xyzw / vPosVS.w;
}

float gauss(float r, float v)
{
    float twoV = 2.0f * v;
    return RCP_PI * (1.0f / twoV) * exp(-(r * r) / twoV);
}

float gauss1D(float x, float variance)
{
    return rcp(sqrt(2.0f * PI) * variance) * exp(-(x * x) * rcp(2.0f * variance * variance));
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

float linearizeDepth(float depth)
{
    float ndcDepth = depth * 2.0f - 1.0f;
    return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - ndcDepth * (FAR_PLANE - NEAR_PLANE));

}


PS_OUT ps(float4 fragPos : SV_Position)
{
    PS_OUT ps_out;
    
    float2 uvCoord = float2(fragPos.x / SCREEN_WIDTH, fragPos.y / SCREEN_HEIGHT);
       
    float centerDepth = (depthMap.Sample(texSampler, uvCoord));
    float linearizedDepth = linearizeDepth(centerDepth) / FAR_PLANE;
    
    float centerDepthLin = (NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + centerDepth * (NEAR_PLANE - FAR_PLANE));
    float4 centerSampleVS = posFromDepth(uvCoord, centerDepth, invCamProjMat);
    //centerSampleVS = mul(centerSampleVS, invCamViewMat);
    
    //float cellSize = 1.0f / 50.0f;
    //float2 cellCoord = trunc(uvCoord * 50.0f) * cellSize;

    float angle = saturate((abs(ddx(centerDepthLin)) + abs(ddy(centerDepthLin))));
    angle /= subsurfaceAlbedo.a;
    angle = pow(angle, meanFreePathColor.a);
    angle = 1.0f - angle;
    //angle = pow(angle, subsurfaceAlbedo.g);

    
    float3 col = float3(0.0f, 0.0f, 0.0f);
    float3 downsampledRad = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, (angle * 7.0f)).rgb, RCP_GAMMA);
    float3 rad = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, subsurfaceAlbedo.b).rgb, RCP_GAMMA); // + pow(transmittedIrrad.Sample(texSampler, uvCoord).rrr, RCP_GAMMA);

    float contrast = 0.0f;
    
    //if (centerDepth < 0.99f)
    //{
    //    contrast = max(pow(0.1f, 1.0f / GAMMA), (rad.r - downsampledRad.r) / downsampledRad.r);
    //}
    
    float4 u = float4(0.0f,0.0f,0.0f,0.0f);
    float3 spec = pow(specularIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA);
    //float maxr = 0.0f;
    
    float3 albedo = pow(gbuffer_albedo.Sample(texSampler, uvCoord).rgb, RCP_GAMMA);
    
       //GOLUBEV (2018)
    if (sssModelId == 3)
    {
        //spec = pow(specularIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA);

        float r;

        float3 dmfp = meanFreePathColor.rgb * meanFreePathDist * scale;
        float maxDmfp = max(dmfp.r, max(dmfp.g, dmfp.b));
        
        float3 A = subsurfaceAlbedo.rgb; //albedo;
        float maxA = max(A.r, max(A.g, A.b));
        
        float s = 3.5f + 100.0f * pow(maxA - 0.33, 4.0f);
        float3 s3 = 3.5f + 100.0f * pow(A - 0.33, 4.0f);
        
        float3 d3 = dmfp / s3;
        float d = maxDmfp / s;

        float3 sumWeights = float3(0.0f, 0.0f, 0.0f);
        float3 sss = float3(0.0f, 0.0f, 0.0f);
        
        float randAngleJitt = pseudoRandom(uvCoord, 1.0f) * PI;
        float cosRandAngle = cos(randAngleJitt);
        float sinRandAngle = sin(randAngleJitt);
        float2x2 randRot = float2x2(cosRandAngle, -sinRandAngle, sinRandAngle, cosRandAngle);
        
        //-------------------------
        float curSamp = 0.0f;
        
        for (float i = 0; i < numSamples; i += 1.0f)
        {
            u = samples[(int) i];
            //u = clamp(u, float4(0.0f, 0.0f, 0.0f, 0.0f), float4(0.98f, 0.98f, 0.98f, 0.98f));
            float4 fourRadiuses = RadiusRootFindByApproximation(d, u);
            float radiuses[4];
            radiuses[0] = fourRadiuses.x;
            radiuses[1] = fourRadiuses.y;
            radiuses[2] = fourRadiuses.z;
            radiuses[3] = fourRadiuses.w;
            
            for (int j = 0; j < 4; j++)
            {
                if (curSamp >= numSamples)
                    break;
                //float theta = FIBONACCI_SEQUENCE_ANGLE(i*4.0f+j);
                //float theta = (GOLDEN_ANGLE * curSamp /*float((i * 4.0f) + j)*/);
                float theta = mad(GOLDEN_ANGLE, curSamp, randAngleJitt); //randAngleJitt; // (GOLDEN_ANGLE * float((i * 4.0f) + j));
                float cosTheta = cos(theta);
                float sinTheta = sin(theta);
            
                //float tmpFib2 = fib2;
                //fib2 = fib1 + fib2;
                //fib1 = tmpFib2;
                
                //float cosTheta = cos((i * 4.0f + j) * G_A);
                //float sinTheta = cos((i * 4.0f + j) * G_A + HPI);
                
                float r = radiuses[j]; //(radiuses[j] / biggestRadSample) * maxDmfp;
                
                float2 dir = float2(cosTheta, sinTheta) * r;
                //dir = mul(randRot, dir) * r;
                dir = NEAR_PLANE * dir / centerDepthLin; //perspective division
                dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
                dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f); //raster coord system dir     
                float2 sampleCoord = uvCoord + dir;
                
                float sampleDepth = depthMap.Sample(texSampler, sampleCoord);
                float3 sampleVS = posFromDepth(sampleCoord, sampleDepth, invCamProjMat);
                //sampleDepth = (NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + sampleDepth * (NEAR_PLANE - FAR_PLANE));
                //float deltaDepth = (centerDepthLin - sampleDepth) / scale;
                
                float r1 = distance(centerSampleVS, sampleVS); //sqrt((deltaDepth * deltaDepth) + (r * r));
                
                //dir = float2(cosTheta, sinTheta);
                //dir = mul(randRot, dir) * r1;
                //dir = NEAR_PLANE * dir / centerDepthLin; //perspective division
                //dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
                //dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f); //raster coord system dir
                
                //sampleCoord = uvCoord + dir;
                float3 rd = pow(diffuseIrrad.SampleLevel(texSampler, sampleCoord, 0.0f).rgb, RCP_GAMMA); // + pow(transmittedIrrad.Sample(texSampler, sampleCoord).rgb, RCP_GAMMA);
                //float rNorm = r / biggestRadSample;
                //int window = (int) (rNorm + 0.2f);
                
                //accumSep[window] += rd;
                //numSmps[window] += 1;
                
                float pdf = pdfBurleyDP(r, d);
                float3 diffusionProfile = computeBurleyDP(r1, d3);
                
                float3 sampleWeight = (diffusionProfile / pdf);
                sumWeights += sampleWeight;
            
                sss.xyz += sampleWeight * rd.rgb;
                
                curSamp += 1.0f;
            }
        }
                
        sss /= sumWeights;
        col = ((sss + pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA)) /** albedo*/) + spec; //multiply with albedo only if havent done pre scattering

    }
    else if(sssModelId == 2)
    {
        //JIMENEZ SEPARABLE (2015)
        if (centerDepth < 0.9999f)
        {
            float s = 10.34f * 0.001f;
            float3 nearVar = float3(0.077f, 0.034f, 0.02f) * s;
            float3 farVar = float3(1.0f, 0.45f, 0.25f) * s;
        
            //gauss1D()
            
            float linearMiddleDepth = linearizeDepth(centerDepth);
        
            float distanceToProjectionWindow = 1.0 / tan(0.5 * VERT_FOV); //radians(SSSS_FOVY));
            float scal = distanceToProjectionWindow / linearMiddleDepth;
            float sssWidth = meanFreePathDist;
	
	        // Calculate the final step to fetch the surrounding pixels:
            float2 finalStep = sssWidth * scal * dir;
            finalStep *= 1.0f; //SSSS_STREGTH_SOURCE; // Modulate it using the alpha channel.
            finalStep *= (1.0f / 3.0f); /// (2.0 * meanFreePathDist); // sssWidth in mm / world space unit, divided by 2 as uv coords are from [0 1]

            // Accumulate the center sample:
            float4 colM = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0), RCP_GAMMA);
            float4 colorBlurred = colM;
            colorBlurred.rgb *= kernel[0].rgb;
            
            for (int i = 1; i < numSamplesJimenezSep; i++)
            {
                // Fetch color and depth for current sample:
                float2 offset = uvCoord + kernel[i].a * finalStep;
                float4 color = pow(diffuseIrrad.SampleLevel(texSampler, offset, 0.0f), RCP_GAMMA);
            
                //Follow surface
                float linearSampleDepth = linearizeDepth(depthMap.Sample(texSampler, offset).r);
                float s = saturate(300.0f * distanceToProjectionWindow *
                               sssWidth * abs(linearMiddleDepth - linearSampleDepth));
                color.rgb = lerp(color.rgb, colM.rgb, s);

                // Accumulate:
                colorBlurred.rgb += kernel[i].rgb * color.rgb;
            }
        
            if (dir.x == 1)
            {
                col = colorBlurred.rgb;

            }
            else
            {
                spec = pow(specularIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA);
                col = ((colorBlurred.rgb + pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA))/* * albedo*/) + spec; //multiply with albedo only if havent done pre scattering
            }
        
        //col += pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA) * albedo;
        }
    }
    else if (sssModelId == 1)
    {
        //JIMENEZ GAUSSIAN (2009)
        if (centerDepth <= 0.999999f)
        {
            float w[7] = { 0.006, 0.061, 0.242, 0.382, 0.242, 0.061, 0.006 };
            float2 finalWidth = 0.0f;
            
            float linDepth = linearizeDepth(centerDepth) / FAR_PLANE;
            float sssLevel = scale;
            float correction = 800; //BETA;
            float gauss_standard_deviation = meanFreePathDist;
            float maxdd = 0.001f;
            
            if (dir.x == 1)
            {
                float s_x = sssLevel / (linDepth + correction * min(abs(ddx(linDepth)), maxdd));
                finalWidth = s_x * gauss_standard_deviation /** pixelSize*/ * dir;
            }
            else
            {
                float s_y = sssLevel / (linDepth + correction * min(abs(ddy(linDepth)), maxdd));
                finalWidth = s_y * gauss_standard_deviation /** pixelSize*/ * dir;
            }

            float2 offset = uvCoord - finalWidth;
            col = float4(0.0, 0.0, 0.0, 1.0);
            
            for (int i = 0; i < 7; i++)
            {
                float3 tap = pow(diffuseIrrad.Sample(texSampler, offset).rgb, RCP_GAMMA);
                col.rgb += w[i] * tap;
                offset += finalWidth / 3.0;
            }
            
            col += spec;

        }
    }
    else  
    {
        if (centerDepth <= 0.999999f)
        {
            col = (pow(diffuseIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA) /** albedo*/) + spec; //multiply with albedo only if havent done pre scattering
        }
    }

    ////GOLUBEV (2018)
    //if (centerDepth <= 0.999999f)
    //{
    //    spec = pow(specularIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA);

    //    float r;

    //    float3 dmfp = meanFreePathColor.rgb * meanFreePathDist;
    //    float maxDmfp = max(dmfp.r, max(dmfp.g, dmfp.b));
        
    //    float3 A = albedo; //pow(subsurfaceAlbedo.rgb, 1); //albedo;
    //    float maxA = max(A.r, max(A.g, A.b));
        
    //    float s = 3.5f + 100.0f * pow(maxA - 0.33, 4.0f);
    //    float3 s3 = 3.5f + 100.0f * pow(A - 0.33, 4.0f);
        
    //    float3 d3 = dmfp / s3;
    //    float d = maxDmfp / s;

    //    float3 sumWeights = float3(0.0f, 0.0f, 0.0f);
    //    float3 sss = float3(0.0f, 0.0f, 0.0f);
        
    //    float randAngleJitt = pseudoRandom(uvCoord, 1.0f) * PI;
    //    float cosRandAngle = cos(randAngleJitt);
    //    float sinRandAngle = sin(randAngleJitt);
    //    float2x2 randRot = float2x2(cosRandAngle, -sinRandAngle, sinRandAngle, cosRandAngle);
        
    //    rad = float3(1.0f, 1.0f, 1.0f) - rad;
    //    downsampledRad = float3(1.0f, 1.0f, 1.0f) - downsampledRad;
    //    rad = pow(rad, 2.0f);
    //    //downsampledRad = pow(downsampledRad, meanFreePathDist);
    //    float enableSSS = saturate(round(((rad - downsampledRad) / downsampledRad) + subsurfaceAlbedo.r)).r;
        
    //    float numSamples = clamp(scale, 1.0f, 16.0f);
    //    u = samples[(int) numSamples - 1.0f];
    //    float4 fourRadiuses = RadiusRootFindByApproximation(d, u);
    //    //float biggestRadSample = fourRadiuses.w;
        
    //    //float3 accumSep[2];
    //    //accumSep[0] = float3(0.0f, 0.0f, 0.0f);
    //    //accumSep[1] = float3(0.0f, 0.0f, 0.0f);
    //    //int numSmps[2];
    //    //numSmps[0] = 0;
    //    //numSmps[1] = 0;
        
    //    //-------------------------
    //    float curSamp = 0.0f;
        
    //    for (float i = 0; i < numSamples; i += 1.0f)
    //    {
    //        u = samples[(int) i];
    //        //u = clamp(u, float4(0.0f, 0.0f, 0.0f, 0.0f), float4(0.98f, 0.98f, 0.98f, 0.98f));
    //        float4 fourRadiuses = RadiusRootFindByApproximation(d, u);
    //        float radiuses[4];
    //        radiuses[0] = fourRadiuses.x;
    //        radiuses[1] = fourRadiuses.y;
    //        radiuses[2] = fourRadiuses.z;
    //        radiuses[3] = fourRadiuses.w;
            
    //        for (int j = 0; j < 4; j++)
    //        {
    //            //float theta = FIBONACCI_SEQUENCE_ANGLE(i*4.0f+j);
    //            //float theta = (GOLDEN_ANGLE * curSamp /*float((i * 4.0f) + j)*/);
    //            float theta = mad(GOLDEN_ANGLE, curSamp, randAngleJitt); //randAngleJitt; // (GOLDEN_ANGLE * float((i * 4.0f) + j));
    //            float cosTheta = cos(theta);
    //            float sinTheta = sin(theta);
            
    //            //float tmpFib2 = fib2;
    //            //fib2 = fib1 + fib2;
    //            //fib1 = tmpFib2;
                
    //            //float cosTheta = cos((i * 4.0f + j) * G_A);
    //            //float sinTheta = cos((i * 4.0f + j) * G_A + HPI);
                
    //            float r = radiuses[j]; //(radiuses[j] / biggestRadSample) * maxDmfp;
                
    //            float2 dir = float2(cosTheta, sinTheta) * r;
    //            //dir = mul(randRot, dir) * r;
    //            dir = NEAR_PLANE * dir / centerDepthLin; //perspective division
    //            dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
    //            dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f); //raster coord system dir     
    //            float2 sampleCoord = uvCoord + dir;
                
    //            float sampleDepth = depthMap.Sample(texSampler, sampleCoord);
    //            float3 sampleVS = posFromDepth(sampleCoord, sampleDepth, invCamProjMat);
    //            //sampleDepth = (NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + sampleDepth * (NEAR_PLANE - FAR_PLANE));
    //            //float deltaDepth = (centerDepthLin - sampleDepth) / scale;
                
    //            float r1 = distance(centerSampleVS, sampleVS); //sqrt((deltaDepth * deltaDepth) + (r * r));
                
    //            //dir = float2(cosTheta, sinTheta);
    //            //dir = mul(randRot, dir) * r1;
    //            //dir = NEAR_PLANE * dir / centerDepthLin; //perspective division
    //            //dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
    //            //dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f); //raster coord system dir
                
    //            //sampleCoord = uvCoord + dir;
    //            float3 rd = pow(diffuseIrrad.SampleLevel(texSampler, sampleCoord, 0.0f).rgb, RCP_GAMMA); // + pow(transmittedIrrad.Sample(texSampler, sampleCoord).rgb, RCP_GAMMA);
    //            //float rNorm = r / biggestRadSample;
    //            //int window = (int) (rNorm + 0.2f);
                
    //            //accumSep[window] += rd;
    //            //numSmps[window] += 1;
                
    //            float pdf = pdfBurleyDP(r, d);
    //            float3 diffusionProfile = computeBurleyDP(r1, d3);
                
    //            float3 sampleWeight = (diffusionProfile / pdf);
    //            sumWeights += sampleWeight;
            
    //            sss.xyz += sampleWeight * rd.rgb;
                
    //            curSamp += 1.0f;
    //        }
    //    }
        
    //    curSamp = 0.0f;
    //    float i2 = numSamples;
    //    numSamples += (trunc(subsurfaceAlbedo.g * 255.0f) * enableSSS);
    //    //numSamples = i * 2.0f* enableSSS;
    //    for (; i2 < numSamples; i2 += 1.0f)
    //    {
    //        u = samples[(int) i2];
    //        //u = clamp(u, float4(0.0f, 0.0f, 0.0f, 0.0f), float4(0.98f, 0.98f, 0.98f, 0.98f));
    //        float4 fourRadiuses = RadiusRootFindByApproximation(d, u);
    //        float radiuses[4];
    //        radiuses[0] = fourRadiuses.x;
    //        radiuses[1] = fourRadiuses.y;
    //        radiuses[2] = fourRadiuses.z;
    //        radiuses[3] = fourRadiuses.w;
            
    //        for (int j = 0; j < 4; j++)
    //        {
    //            //float theta = FIBONACCI_SEQUENCE_ANGLE(i*4.0f+j);                
    //            //float theta = (GOLDEN_ANGLE * curSamp /*float((i * 4.0f) + j)*/);
    //            float theta = mad(GOLDEN_ANGLE, curSamp, randAngleJitt); //randAngleJitt; // 
    //            float cosTheta = cos(theta);
    //            float sinTheta = sin(theta);
            
    //            //float tmpFib2 = fib2;
    //            //fib2 = fib1 + fib2;
    //            //fib1 = tmpFib2;
                
    //            //float cosTheta = cos((i * 4.0f + j) * G_A);
    //            //float sinTheta = cos((i * 4.0f + j) * G_A + HPI);
                
    //            float r = radiuses[j]; //(radiuses[j] / biggestRadSample) * maxDmfp;
                
    //            float2 dir = float2(cosTheta, sinTheta) * r;
    //            //dir = mul(randRot, dir) * r;
    //            dir = NEAR_PLANE * dir / centerDepthLin; //perspective division
    //            dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
    //            dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f); //raster coord system dir     
    //            float2 sampleCoord = uvCoord + dir;
                
    //            float sampleDepth = depthMap.Sample(texSampler, sampleCoord);
    //            float3 sampleVS = posFromDepth(sampleCoord, sampleDepth, invCamProjMat);
    //            //sampleDepth = (NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + sampleDepth * (NEAR_PLANE - FAR_PLANE));
    //            //float deltaDepth = (centerDepthLin - sampleDepth) / scale;
                

                
    //            float r1 = distance(centerSampleVS, sampleVS); //sqrt((deltaDepth * deltaDepth) + (r * r));
                
    //            //dir = float2(cosTheta, sinTheta);
    //            //dir = mul(randRot, dir) * r1;
    //            //dir = NEAR_PLANE * dir / centerDepthLin; //perspective division
    //            //dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
    //            //dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f); //raster coord system dir
                
    //            //sampleCoord = uvCoord + dir;
    //            float3 rd = pow(diffuseIrrad.SampleLevel(texSampler, sampleCoord, 0.0f).rgb, RCP_GAMMA); // + pow(transmittedIrrad.Sample(texSampler, sampleCoord).rgb, RCP_GAMMA);
    //            //float rNorm = r / biggestRadSample;
    //            //int window = (int) (rNorm + 0.2f);
                
    //            //accumSep[window] += rd;
    //            //numSmps[window] += 1;
                
    //            float pdf = pdfBurleyDP(r, d);
    //            float3 diffusionProfile = computeBurleyDP(r1, d3);
                
    //            float3 sampleWeight = (diffusionProfile / pdf);
    //            sumWeights += sampleWeight;
            
    //            sss.xyz += sampleWeight * rd.rgb;
    //            curSamp += 1.0f;
    //        }
    //    }
        
    //    sss /= sumWeights;
    //    col = ((sss + /*pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA) +*/pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA)) * albedo)+spec;
    //    //col = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA) * albedo;
    //    //contrast = max(pow(float3(0.0f, 0.0f, 0.0f), RCP_GAMMA), abs(rad.r - downsampledRad.r) / downsampledRad.r);
    //    //col += pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA);
    //    //col = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA);
    //    //col += pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA);
    //    //accumSep[0] = saturate(accumSep[0]);
    //    //accumSep[1] = saturate(accumSep[1]);
    //    //float3 aIn = accumSep[0] / numSmps[0];
    //    //float3 aOut = (accumSep[1] / numSmps[1]);
    //    //rad = float3(1.0f, 1.0f, 1.0f) - rad;
    //    //downsampledRad = float3(1.0f, 1.0f, 1.0f) - downsampledRad;

    //    //col = float3(angle,angle,angle); //enableSSS;
    //    //col = downsampledRad;
    //    //col = rad;
    //    //col = ((rad - downsampledRad) / downsampledRad) ; //abs(aIn - aOut) / aOut;
    //    //col = float3(enableSSS, enableSSS, enableSSS);
    //}

    ////contrast = ((clamp(contrast, 0.5f, 1.0f) - 0.5f) * 2) > 0.2f ? 1.0f : 0.0f;
    
    //JIMENEZ SEPARABLE (2015)
 //   if (centerDepth < 0.9999f)
 //   {
 //       float s = 10.34f * 0.001f;
 //       float3 nearVar = float3(0.077f, 0.034f, 0.02f) * s;
 //       float3 farVar = float3(1.0f, 0.45f, 0.25f) * s;
        
 //       //gauss1D()
        
 //       float distanceToProjectionWindow = 1.0 / tan(0.5 * VERT_FOV); //radians(SSSS_FOVY));
 //       float scal = distanceToProjectionWindow / centerDepthLin;
 //       float sssWidth = 1.0f;
	
	//// Calculate the final step to fetch the surrounding pixels:
 //       float2 finalStep = sssWidth * scal * dir;
 //       finalStep *= 1.0f; //SSSS_STREGTH_SOURCE; // Modulate it using the alpha channel.
 //       finalStep *= 1.0 / (2.0 * meanFreePathDist); // sssWidth in mm / world space unit, divided by 2 as uv coords are from [0 1]
	
 //       float4 colM = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0), RCP_GAMMA);
 //       float depthM = centerDepth;
        
 //   // Accumulate the center sample:
 //       float4 colorBlurred = colM;
 //       colorBlurred.rgb *= kernel[0].rgb;
        
 //       for (int i = 1; i < 7; i++)
 //       {
 //       // Fetch color and depth for current sample:
 //           float2 offset = uvCoord + kernel[i].a * finalStep;
 //           float4 color = pow(diffuseIrrad.SampleLevel(texSampler, offset, 0.0f), RCP_GAMMA);
            
 //           //Follow surface
 //           float depth = depthMap.Sample(texSampler, offset);
 //           float s = saturate(300.0f * distanceToProjectionWindow *
 //                              sssWidth * abs(depthM - depth));
 //           color.rgb = lerp(color.rgb, colM.rgb, s);

 //       // Accumulate:
 //           colorBlurred.rgb += kernel[i].rgb * color.rgb;
 //       }
        
 //       if (dir.x == 0)
 //       {
 //           col = colorBlurred;
 //       }
 //       else
 //       {
 //           spec = pow(specularIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, RCP_GAMMA);
 //           col = ((colorBlurred.rgb + pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA)) * albedo) + spec;
 //       }
        
 //       //col += pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA) * albedo;
 //   }

    ////JIMENEZ SEPARABLE (2009)
    //if (centerDepth < 0.9999f)
    //{
    //    spec = pow(specularIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, 1.0f / GAMMA);
        
    //    for (int j = 0; j < 6; j++)
    //    {
    //        float netFilterWidth = (1 / SCREEN_WIDTH) * gaussWidths[j] * sx;
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
    //            float3 ird = pow(diffuseIrrad.SampleLevel(texSampler, coords, 0).rgb, RCP_GAMMA); // + pow(transmittedIrrad.Sample(texSampler, coords).rgb, 1.0f / GAMMA);
    //            ird *= albedo;
    //            col += (gaussian[i] * (ird * weights[j])) * 0.5f;
    //            coords += float2(netFilterWidth, 0.0f);
    //        }
        
    //        netFilterWidth = (1 / SCREEN_HEIGHT) * gaussWidths[j] * sy;
    //        coords = uvCoord - float2(0.0f, netFilterWidth * 3.0);
    //        for (int i2 = 0; i2 < 7; i2++)
    //        {
    //            float3 ird = pow(diffuseIrrad.SampleLevel(texSampler, coords, 0).rgb, RCP_GAMMA); //+pow(transmittedIrrad.Sample(texSampler, coords).rgb, 1.0f / GAMMA);
    //            ird *= albedo;
    //            col += (gaussian[i2] * ird * weights[j]) * 0.5f;
    //            coords += float2(0.0f, netFilterWidth);
    //        }
    //    }
        
    //    //col += pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA) * albedo;
    //}
    
    ps_out.lastBiggestBlur = float4(pow(col /* + spec*/, GAMMA), 1.0f);
    ps_out.finalComposited = float4(pow(col /* + spec*/, GAMMA), 1.0f);
    
    return ps_out;
    
    }
