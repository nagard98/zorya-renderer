#include "common/LightStruct.hlsli"

#define FIBONACCI_SEQUENCE_ANGLE(x) (((float(x) + 0.5)*(1 + sqrt(5))*0.5) * 2 * PI)
#define G_A 2.399963
#define HPI 1.570796
#define INNER_NEIGHBOR 0
#define OUTER_NEIGHBOR 1
#define SSS_SUPERSAMPLES 13

static const int numSamplesJimenezSep = 9;

#define MaterialTexture Texture2D

Texture2D diffuseIrrad : register(t0);
Texture2D specularIrrad : register(t1);
Texture2D transmittedIrrad : register(t2);
Texture2D depthMap : register(t3);
Texture2D gbuffer_albedo : register(t4);

//For final composit
//Texture2D sss_ex_rad : register(t5);

SamplerState texSampler : register(s0);

struct PS_OUT
{
    float4 finalComposited : SV_Target0;
    float4 lastBiggestBlur : SV_Target1;
};

struct ModelSSS
{
    //golubev subsurface scattering model
    float4 samples[16];
    float4 subsurfaceAlbedo;
    float4 meanFreePathColor;
    
    float meanFreePathDist;
    float scale;
    
    float2 dir;
    int numSamples;
    int num_supersamples;
    int2 pad2;

    //golubev subsurface scattering model
    float4 kernel[15];
    //float4 kernel[16];
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
    
    ModelSSS sssModels[64];
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
    float2 dir;
    int shader_sss_model_id;
    int pad5;
    float sd_gauss_jim;
    float pad4;
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

static const float FAR_PLANE = 100.0f;
static const float NEAR_PLANE = 0.01f;
static const float Q = FAR_PLANE / (FAR_PLANE - NEAR_PLANE);
static const float ALPHA = 3.0f;
static const float BETA = 800.0f;


//static const float SCREEN_WIDTH = 1280.0f;
//static const float SCREEN_HEIGHT = 720.0f;
static const float SCREEN_WIDTH = 1920.0f;
static const float SCREEN_HEIGHT = 1080.0f;
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

//float4 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat)
//{
//    float x = quadTexCoord.x * 2.0f - 1.0f;
//    float y = (1.0f - quadTexCoord.y) * 2.0f - 1.0f;
    
//    float4 vProjPos = float4(x, y, sampledDepth, 1.0f);
//    float4 vPosVS = mul(vProjPos, invMat);
    
//    return vPosVS.xyzw / vPosVS.w;
//}


float3 posFromDepth(float2 quadTexCoord, float sampledDepth, uniform float4x4 invMat)
{
    float x = quadTexCoord.x * 2.0f - 1.0f;
    float y = (1.0f - quadTexCoord.y) * 2.0f - 1.0f;
    
    float4 vProjPos = float4(x, y, sampledDepth, 1.0f);
    float4 vPosVS = mul(vProjPos, invMat);
    
    return vPosVS.xyz / vPosVS.w;
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

struct Sample_Info
{
    float4 pos;
    float depth;
};

struct Burley_SSS_State
{
    float3 weights_sum;
    float3 sss;
    float curSamp;
};

struct Burley_Plus_SSS_State
{
    float3 weights_sum;
    float curSamp;
    float3 sss;
    float3 neighbor[2];
};

struct Burley_SSS_Params
{
    float3 d3;
    float d;
};
    
struct Burley_Plus_SSS_Params
{
    float3 d3;
    float d;
    float sample_split_index;
    
};

Burley_Plus_SSS_State burley_plus_sample_scene(uint index_first_pack, uint num_sample_packs, float idx_last_sample, float randAngleJitt, float2 uvCoord, Burley_Plus_SSS_State sss_state, Burley_Plus_SSS_Params sss_params, Sample_Info centerSampleLinInfo, ModelSSS currentSSSModel)
{
    
    for (uint i = index_first_pack; i < num_sample_packs; i += 1)
    {
        float4 pack_rand_numbers = currentSSSModel.samples[i];
        float radiuses[4] = (float[4]) RadiusRootFindByApproximation(sss_params.d, pack_rand_numbers);
            
        for (int j = 0; j < 4; j++)
        {
            if (sss_state.curSamp > idx_last_sample)
                break;

            float theta = mad(GOLDEN_ANGLE, sss_state.curSamp, randAngleJitt);
            float cosTheta = cos(theta);
            float sinTheta = sin(theta);
                
            float r = radiuses[j];
                
            float2 dir = float2(cosTheta, sinTheta) * r;
            dir = NEAR_PLANE * dir / centerSampleLinInfo.depth; //perspective division
            dir = (dir + (NP_SIZE / 2.0f)) / NP_SIZE; //normalized dir
            dir = float2(dir.x - 0.5f, (1.0f - dir.y) - 0.5f); //raster coord system dir     
            float2 sampleCoord = uvCoord + dir;
                
            float sampleDepth = depthMap.SampleLevel(texSampler, sampleCoord, 0).r;
            float3 sampleVS = posFromDepth(sampleCoord, sampleDepth, invCamProjMat).xyz;
                
            float r1 = distance(centerSampleLinInfo.pos.xyz, sampleVS); //sqrt((deltaDepth * deltaDepth) + (r * r));
                
            float3 rd = diffuseIrrad.SampleLevel(texSampler, sampleCoord, 0.0f).rgb; // + pow(transmittedIrrad.Sample(texSampler, sampleCoord).rgb, RCP_GAMMA);
            uint idx_neighbor = (uint) saturate(trunc(sss_state.curSamp * rcp(sss_params.sample_split_index)));
            if (sss_state.curSamp <= sss_params.sample_split_index)
            {
                sss_state.neighbor[INNER_NEIGHBOR] += rd;
            }
            else
            {
                sss_state.neighbor[OUTER_NEIGHBOR] += rd;
            }
                
            
            float pdf = pdfBurleyDP(r, sss_params.d);
            float3 diffusionProfile = computeBurleyDP(r1, sss_params.d3);
                
            float3 sampleWeight = (diffusionProfile / pdf);
            sss_state.weights_sum += sampleWeight;
            
            sss_state.sss.rgb += sampleWeight * rd.rgb;
                
            sss_state.curSamp += 1.0f;
        }
    }
    
    return sss_state;
}


PS_OUT ps(float4 fragPos : SV_Position)
{
    PS_OUT ps_out;
    
    float3 col = float3(0.0f, 0.0f, 0.0f);
    
    float2 uvCoord = float2(fragPos.x / SCREEN_WIDTH, fragPos.y / SCREEN_HEIGHT);
    
    float4 albedo_sample = gbuffer_albedo.Sample(texSampler, uvCoord).rgba;
    float3 albedo = albedo_sample.rgb;
    
    uint sssParamsPack = albedo_sample.a * 255.0f;
    uint pixel_sss_model_id = ((sssParamsPack >> 6) & 3);
    
    //if (albedo_sample.a == 1.0f)
    //{
    //    if (shader_sss_model_id == 0)
    //    {
    //        ps_out.lastBiggestBlur = albedo_sample;
    //        ps_out.finalComposited = albedo_sample;
    //    }
    //    else
    //    {
    //        //TODO: if albedo alpha in sss is 1.0f, then its the background (e.g. skybox)
    //        discard;
    //    }

    //    return ps_out;
    //}
    
    //TODO: move ModelSSS to other constant buffer that is not updated per frame and allow full 256 models
    ModelSSS currentSSSModel = sssModels[sssParamsPack & 63];
       
    Sample_Info center_sample;
    float centerDepth = (depthMap.SampleLevel(texSampler, uvCoord,0).r);
    float4 centerSampleVS = float4(posFromDepth(uvCoord, centerDepth, invCamProjMat), 1.0f);
    center_sample.pos = centerSampleVS/*posFromDepth(uvCoord, centerDepth, invCamProjMat)*/;
    
    float linearizedDepth = linearizeDepth(centerDepth) / FAR_PLANE;
    float centerDepthLin = (NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + centerDepth * (NEAR_PLANE - FAR_PLANE));
    center_sample.depth = (NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + centerDepth * (NEAR_PLANE - FAR_PLANE));

    float4 pack_rand_numbers = float4(0.0f,0.0f,0.0f,0.0f);

    ////GOLUBEV (2018)
    //if (shader_sss_model_id == 3)
    //{
        if (pixel_sss_model_id == 3)
        {
            float3 dmfp = currentSSSModel.meanFreePathColor.rgb * currentSSSModel.meanFreePathDist * currentSSSModel.scale;
            float maxDmfp = max(dmfp.r, max(dmfp.g, dmfp.b));
        
            float3 A = currentSSSModel.subsurfaceAlbedo.rgb; //albedo;
            float maxA = max(A.r, max(A.g, A.b));
        
            float s = 3.5f + 100.0f * pow(maxA - 0.33, 4.0f);
            float3 s3 = 3.5f + 100.0f * pow(A - 0.33, 4.0f);
        
            Burley_Plus_SSS_Params sss_params;
            sss_params.d = maxDmfp / s;
            sss_params.d3 = dmfp / s3;
            sss_params.sample_split_index = trunc(currentSSSModel.numSamples * 0.3f);

            Burley_Plus_SSS_State sss_state;
            sss_state.curSamp = 0.0f;
            sss_state.sss = float3(0.0f, 0.0f, 0.0f);
            sss_state.weights_sum = float3(0.0f, 0.0f, 0.0f);
            sss_state.neighbor[INNER_NEIGHBOR] = diffuseIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb;
            sss_state.neighbor[OUTER_NEIGHBOR] = float3(0, 0, 0);
        
            float randAngleJitt = pseudoRandom(uvCoord, 2.0f) * 2.0f * PI;
            float cosRandAngle = cos(randAngleJitt);
            float sinRandAngle = sin(randAngleJitt);
            float2x2 randRot = float2x2(cosRandAngle, -sinRandAngle, sinRandAngle, cosRandAngle);
        
        //-------------------------

            int num_sample_packs = round((currentSSSModel.numSamples * rcp(4.0f)) + 0.5f);
            num_sample_packs = clamp(num_sample_packs, 1, 10);
        
            sss_state = burley_plus_sample_scene(0, num_sample_packs, currentSSSModel.numSamples, randAngleJitt, uvCoord, sss_state, sss_params, center_sample, currentSSSModel);
        
            //sss_state.neighbor[OUTER_NEIGHBOR] += sss_state.neighbor[INNER_NEIGHBOR];
            
            //sss_state.neighbor[INNER_NEIGHBOR] *= rcp(sss_params.sample_split_index + 2);
            //sss_state.neighbor[OUTER_NEIGHBOR] *= rcp((float)currentSSSModel.numSamples); // - (sss_params.sample_split_index + 1));
        
            //float3 local_contrast = abs(sss_state.neighbor[OUTER_NEIGHBOR] - sss_state.neighbor[INNER_NEIGHBOR]) * rcp(sss_state.neighbor[OUTER_NEIGHBOR] + sss_state.neighbor[INNER_NEIGHBOR]);
            //local_contrast = round(saturate(local_contrast + currentSSSModel.meanFreePathColor.a));
            
            //if (any(local_contrast))
            //{
            //    float idx_last_supersampling_sample = currentSSSModel.numSamples + currentSSSModel.num_supersamples;
            //    int num_supersampling_packs = round((currentSSSModel.num_supersamples * rcp(4.0f)) + 0.5f);
                
            //    randAngleJitt = pseudoRandom(uvCoord, 4.3619f) * 2.0f * PI;
            //    sss_state.curSamp = 0.0f;
                
            //    sss_state = burley_plus_sample_scene(8, 8 + num_supersampling_packs, currentSSSModel.num_supersamples, randAngleJitt, uvCoord, sss_state, sss_params, center_sample, currentSSSModel);
            //}
        
            //if (round(saturate(currentSSSModel.subsurfaceAlbedo.a)) == 0.0f)
            //{
                sss_state.sss /= sss_state.weights_sum;
                col = sss_state.sss /** albedo*/; //multiply with albedo only if havent done pre scattering
            //}
            //else
            //{
            //    col = any(local_contrast) ? float3(1.0f, 1.0f, 1.0f) : float3(0.0f, 0.0f, 0.0f);
            //}
            
            float3 spec = specularIrrad.SampleLevel(texSampler, uvCoord, 0).rgb;
            
            ps_out.lastBiggestBlur = float4(col + spec, 1.0f);
            ps_out.finalComposited = float4(col + spec, 1.0f);
        
        
        }
        //else
        //{
        //    discard;
        //}
            return ps_out;
        

    //}
    //else if (shader_sss_model_id == 2)
    //{
    //    //JIMENEZ SEPARABLE (2015)
    //    if (pixel_sss_model_id == 2)
    //    {            
    //        float linearMiddleDepth = linearizeDepth(centerDepth);
        
    //        float distanceToProjectionWindow = 1.0 / tan(0.5 * VERT_FOV); //radians(SSSS_FOVY));
    //        float scal = distanceToProjectionWindow / linearMiddleDepth;
    //        float sssWidth = currentSSSModel.meanFreePathDist;
            
    //        float rot_threshold = sssWidth * 1000.0f * currentSSSModel.scale;
	
	   //     // Calculate the final step to fetch the surrounding pixels:
    //        float finalStep = sssWidth * scal /** dir*/;
    //        //finalStep *= 1.0f; //SSSS_STREGTH_SOURCE; // Modulate it using the alpha channel.
    //        finalStep *= (1.0f / 6.0f); /// (2.0 * meanFreePathDist); // sssWidth in mm / world space unit, divided by 2 as uv coords are from [0 1]

    //        // Accumulate the center sample:
    //        float4 colM = diffuseIrrad.SampleLevel(texSampler, uvCoord, 0);
    //        float4 colorBlurred = colM;
    //        colorBlurred.rgb *= currentSSSModel.kernel[0].rgb;
            
    //        float3 weight_accum = 0.0001f + currentSSSModel.kernel[0].rgb;
            
    //        float mask[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
            
    //        float2 new_axis = float2(0.0f, 0.0f);
    //        if (dir.x == 1.0f)
    //        {
    //            float randAngleJitt = (pseudoRandom(uvCoord, 1.0f) * PI) - (0.5f * PI);
    //            float cosRandAngle = cos(randAngleJitt);
    //            float sinRandAngle = sin(randAngleJitt);
    //            new_axis = float2(cosRandAngle, sinRandAngle);
    //        }
    //        else
    //        {
    //            float randAngleJitt = (pseudoRandom(uvCoord, 1.0f) * PI) + (0.5f * PI);
    //            float cosRandAngle = cos(randAngleJitt);
    //            float sinRandAngle = sin(randAngleJitt);
    //            new_axis = float2(cosRandAngle, sinRandAngle);
    //        }
            
    //        for (int i = 1; i < numSamplesJimenezSep; i++)
    //        {
    //            float sample_dist = currentSSSModel.kernel[i].a;

    //            float2 jittered_dir = abs(sample_dist) < rot_threshold ? new_axis * finalStep : dir * finalStep;
    //            jittered_dir *= sample_dist;
                
    //            // Fetch color and depth for current sample:
    //            float2 offset = uvCoord + jittered_dir;
    //            float4 color = diffuseIrrad.SampleLevel(texSampler, offset, 0.0f);
    //            uint sssPack = gbuffer_albedo.SampleLevel(texSampler, uvCoord, 0).a * 255.0f;
    //            uint pixel_sss_model_id = (sssPack & 3);
            
    //            //Follow surface
    //            float linearSampleDepth = linearizeDepth(depthMap.SampleLevel(texSampler, offset, 0.0f).r);
    //            float s = saturate(300.0f * distanceToProjectionWindow *
    //                           sssWidth * abs(linearMiddleDepth - linearSampleDepth));
    //            s = 1 - s;
    //            s *= mask[pixel_sss_model_id];
    //            color.rgb = lerp(colM.rgb, color.rgb, s);

    //            // Accumulate:
    //            float3 local_weight = currentSSSModel.kernel[i].rgb;
    //            colorBlurred.rgb += local_weight * color.rgb;
    //            weight_accum += local_weight;
    //        }
        
    //        col = colorBlurred.rgb; /// weight_accum;
            
    //        //col += pow(transmittedIrrad.Sample(texSampler, uvCoord).rgb, RCP_GAMMA) * albedo;
    //        ps_out.lastBiggestBlur = float4(col, 1.0f);
    //        ps_out.finalComposited = float4(col, 1.0f);
    //    }
    //    else
    //    {
    //        discard;
    //    }
        
    //    return ps_out;
    ////}
    ////else if (shader_sss_model_id == 1)
    ////{
    //    //JIMENEZ GAUSSIAN (2009)
    //    if (pixel_sss_model_id == 1)
    //    {
    //        float w[7] = { 0.006, 0.061, 0.242, 0.382, 0.242, 0.061, 0.006 };
    //        float2 finalWidth = 0.0f;
            
    //        float linDepth = linearizeDepth(centerDepth);
    //        float sssLevel = currentSSSModel.scale;
    //        float correction = currentSSSModel.meanFreePathDist; //BETA;
    //        float pixelSize = currentSSSModel.subsurfaceAlbedo.a;
    //        float maxdd = 0.001f;
            
    //        float scale_factor = 0.0f;
            
    //        if (dir.x == 1)
    //        {
    //            float dddep = ddx(linDepth);
    //            scale_factor = sssLevel / (linDepth + correction * min(abs(dddep), maxdd));
    //        }
    //        else
    //        {
    //            float dddep = ddy(linDepth);
    //            scale_factor = sssLevel / (linDepth + correction * min(abs(dddep), maxdd)); 
    //        }
            
    //        finalWidth = scale_factor * sd_gauss_jim * pixelSize * dir;

    //        float2 offset = uvCoord - finalWidth;
    //        col = float3(0.0, 0.0, 0.0);
            
    //        float mask[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
            
    //        for (int i = 0; i < 7; i++)
    //        {
    //            uint sssPack = gbuffer_albedo.SampleLevel(texSampler, offset, 0).a * 255.0f;
    //            uint pixel_sss_model_id = (sssPack & 3);
                
    //            float3 tap = diffuseIrrad.SampleLevel(texSampler, offset, 0).rgb;
    //            col.rgb += w[i] * tap * mask[pixel_sss_model_id];
    //            offset += finalWidth / 3.0;
    //        }
            
    //        //col += spec;
    //        //float dd = min(abs(ddx(linDepth)), maxdd) + min(abs(ddy(linDepth)), maxdd);
    //        //scale_factor /= 10000.0f;
    //        //col = float3(scale_factor, scale_factor, scale_factor);
            
    //        ps_out.lastBiggestBlur = float4(col, 1.0f);
    //        ps_out.finalComposited = float4(col, 1.0f);
    //    }
    //    else
    //    {
    //        discard;
    //    }
        
    //    return ps_out;
    ////}
    ////else /*if (shader_sss_model_id == 0)*/
    ////{
    //    float3 spec = specularIrrad.SampleLevel(texSampler, uvCoord, 0).rgb;
    //    //albedo = float3(1.0f, 1.0f, 1.0f);
        
    //    if (pixel_sss_model_id == 0)
    //    {
    //        col = mad(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0).rgb, albedo, spec);
    //    }
    //    else
    //    {
    //        col = mad(sss_ex_rad.SampleLevel(texSampler, uvCoord, 0).rgb, albedo, spec); //multiply with albedo only if havent done pre scattering
    //    }
        
    //    ps_out.lastBiggestBlur = float4(col, 1.0f);
    //    ps_out.finalComposited = float4(col, 1.0f);
        
    //    return ps_out;
    //}
    
    }
