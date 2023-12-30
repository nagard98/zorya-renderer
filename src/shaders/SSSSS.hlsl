Texture2D diffuseIrrad : register(t0);
Texture2D specularIrrad : register(t1);
Texture2D transmittedIrrad : register(t2);
Texture2D depthMap : register(t3);

SamplerState texSampler : register(s0);

static const float PI = 3.14159265f;
static const float revPi = 1 / PI;
static const float gamma = 1.0f / 2.2f;
static const float LOG2_E = log2(2.71828f);
static const float RCP_PI_8 = 1.0f / (3.14159f * 8.0f);

static const float zf = 100.0f;
static const float zn = 1.0f;
static const float q = zf / (zf - zn);
static const float alpha = 11.0f;
static const float beta = 800.0f;

static const float gaussWidths[6] = { 0.08f, 0.22f, 0.4324f, 0.7529f, 1.4106f, 2.722f };
static const float3 weights[6] = { 
    float3(0.233f, 0.455f, 0.649f),
    float3(0.100f, 0.336f, 0.344f),
    float3(0.118f, 0.196f, 0.0f),
    float3(0.113f, 0.007f, 0.007f),
    float3(0.358f, 0.004f, 0.0f),
    float3(0.078f, 0.0f, 0.0f),
};

float rand(float x)
{
    return frac(sin(x) * 100000.0f);
}

float rand_1_05(in float2 uv)
{
    float2 noise = (frac(sin(dot(uv, float2(12.9898, 78.233) * 2.0)) * 43758.5453));
    return abs(noise.x + noise.y) * 0.5;
}

void sampleBurleyDP(float u, float rcpS, out float r, out float rcpPdf)
{
    u = 1 - u; // Convert CDF to CCDF; the resulting value of (u != 0)

    float g = 1.0f + (4.0f * u) * (2.0f * u + sqrt(1.0f + (4.0f * u) * u));
    float n = exp2(log2(g) * (-1.0f / 3.0f)); 
    float p = (g * n) * n;
    float c = 1.0f + p + n;
    float x = (3.0f / log2(2.718f)) * log2(c * rcp(4.0f * u));

    float rcpExp = ((c * c) * c) * rcp((4.0f * u) * ((c * c) + (4.0f * u) * (4.0f * u)));

    r = x * rcpS;
    rcpPdf = (8.0f * 3.14159f * rcpS) * rcpExp;
}

float3 computeBurleyDP(float r, float3 scatDist)
{
    //float d = s; //rcp(s);
    return (exp(-r / scatDist) + exp(-r / (3.0f * scatDist))) / (8.0f * PI * scatDist) / r;
    //return rcp(8.0f * 3.14f * d * r) * ;
}

float3 pdfBurleyDP(float r, float3 scatDist)
{
    return (exp(-r / scatDist) + exp(-r / (3.0f * scatDist))) / (8.0f * PI * scatDist);
}

float pseudoRandom(float2 uv, float time)
{
    float value = sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453 + time;
    return frac(sin(value) * 143758.5453);
}

float4 vs(uint vId : SV_VertexID) : SV_Position
{
    float2 texCoord = float2(vId & 1, vId >> 1);
    return float4((texCoord.x - 0.5f) * 2.0f, -(texCoord.y - 0.5f) * 2.0f, 0.0f, 1.0f);
}

float gauss(float r, float v)
{
    float twoV = 2.0f * v;
    return revPi * (1.0f / twoV) * exp(-(r * r) / twoV);
}

float f0(float y, float s, float r)
{
    return 1.0f - (1.0f / 4.0f) * exp(-s * y) - (3.0f / 4.0f) * exp((-s * y) / 3.0f) - r;
}

float f1(float y, float s)
{
    return (1.0f / 4.0f) * s * exp(-s * y) + (s / 4.0f) * exp(-(s * y) / 3.0f);
}

float f2(float y, float s)
{
    return -(1.0f / 4.0f) * s * s * exp(-s * y) + ((s * s) / 12.0f) * exp(-(s * y) / 3.0f);
}

float f_f(float y_k, float s, float r)
{
    return (f0(y_k, s, r)) / (f1(y_k, s) - 0.5f * ((f0(y_k, s, r) * f2(y_k, s)) / f1(y_k, s)));
}

float findZero(float y, float s, float r)
{
    float f0_y = f0(y, s, r);
    float y_k = y;
    while (abs(f0_y) > 0.000001f)
    {
        float y_k1 = y_k - (f0(y_k, s, r)) / (f1(y_k, s) - 0.5f * ((f0(y_k, s, r) * f2(y_k, s)) / f1(y_k, s)));
        
        f0_y = f0(y_k1, s, r);
        y_k = y_k1;
    }
    
    return y_k;
}


float4 ps(float4 fragPos : SV_Position) : SV_Target
{
    float2 uvCoord = float2(fragPos.x / 1280.0f, fragPos.y / 720.0f);
       
    float sampledDepth = (depthMap.Sample(texSampler, uvCoord));
    float depth = (zn * zf) / (zf + sampledDepth * (zn - zf));
    
    float cellSize = 1.0f / 50.0f;
    float2 cellCoord = trunc(uvCoord * 50.0f) * cellSize;
    //float depth = depthMap.Sample(texSampler, uvCoord);
    //depth = (-zn * q) / (depth - q);

    float sx = alpha / (depth + beta * abs(ddx(depth)));
    float sy = alpha / (depth + beta * abs(ddy(depth)));
    
    float2 d = (float2(sx,sy) + 1.0f)/2.0f;
    
    float3 col = float3(0.0f, 0.0f, 0.0f);
    
    float3 rad2 = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 3.0f).rgb, 1.0f / gamma);
    float3 rad = pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, 1.0f / gamma) + pow(transmittedIrrad.Sample(texSampler, uvCoord).rrr, 1.0f / gamma);
    float drx = abs(ddx(rad.r));
    float dry = abs(ddy(rad.r)); 

    
    float maxRad = 0.0f;
    float minRad = 1.0f;
    
    //float t = 0.0f;
    float contrast = 0.0f;
    //col = float3(0.0f, 0.0f, 0.0f);
    if (sampledDepth < 0.99f)
    {
        //t = max(pow(0.1f, 1.0f / gamma), pow(0.1f, 1.0f / gamma) + drx + dry);
        contrast = max(pow(0.1f, 1.0f / gamma), (rad.r - rad2.r) / rad2.r);
        //contrast = max(pow(float3(0.1f,0.1f,0.1f), 1.0f / gamma), (rad - rad2) / rad2);
        //col = float3(d.x, d.y, 0.0f);
    }
    
        //float t = 0.0f;
    float contrast2 = 0.0f;
    //col = float3(0.0f, 0.0f, 0.0f);
    
    float u = 0.0f;
    float I = 0.0f;

    
    if (sampledDepth < 0.99f)
    {
        float3 scatDistances = float3(0.16069999999f, 0.07377413871f, 0.07377413871f);
        float scatDist = scatDistances.r;
        float maxRadius = scatDist;
        float r;
        float rcpPdf;

        float G_A = 2.399963f;
        float HPI = 1.570796f;

        float dmfp = 2.6748f;
        
        float A = 0.8f;
        float s = 3.5f + 100.0f * pow(A - 0.33, 4.0f);
        float d = dmfp / s;
        
        float3 sss = float3(0.0f, 0.0f, 0.0f);
        float3 weight = float3(0.0f, 0.0f, 0.0f);
        
        float numSamples = 32.0f;
        for (float i = 0; i < numSamples; i+=1.0f)
        {
            u = clamp(pseudoRandom(uvCoord, A), 0.0001f, 0.9999f);
            sampleBurleyDP(u, scatDist, r, rcpPdf);
            //float r = findZero(0.1f, scatDist, u) * maxRadius;
            r *= maxRadius;
            
            float2 dir = normalize(cos(i * G_A + float2(0, HPI)) * sqrt(i / numSamples)) * r;
            dir = float2((dir.x * sx) / 1280.0f, dir.y / 720.0f);
            //float angle1 = rand((float) i);
            //float angle2 = rand((float) i * 20.0f);
            //float angle = (1.0f / i) * 3.14f * 2.0f;
            
            //float dir = float2(cos(angle) / 1280.0f, sin(angle) / 720.0f)*3.0f; //* r;
            float2 newCoord = uvCoord + dir;
            float3 rd = pow(diffuseIrrad.SampleLevel(texSampler, newCoord, 0.0f).rgb, 1.0f / gamma); /*+ pow(transmittedIrrad.Sample(texSampler, newCoord).rgb, 1.0f / gamma);*/

            float radius = r * sx;
            //float pdf = pdfBurleyDP(r, (1.0f/scatDist));
            float3 R = computeBurleyDP(0.5f, scatDistances);
            float3 pdf = pdfBurleyDP(r, float3(scatDist, scatDist, scatDist));
            
            //float p = R * rcpPdf;
            
            //if (p > 10.0f)
            //{
            //    I = 0.0f;
            //    break;
            //}
            //I = 1.0f;
            sss += (R / pdf) * rd;
            weight += R / pdf;
        }
        sss = sss / max(weight, 0.0001f);
        //I *= (/*(2.0f * 3.14f)*/ 1.0f/numSamples);

        //t = max(pow(0.1f, 1.0f / gamma), pow(0.1f, 1.0f / gamma) + drx + dry);
        //contrast = clamp(max(pow(0.1f, 1.0f / gamma), (rad.r - rad2.r) / rad2.r), 0.5f, 1.0f);
        contrast2 = max(pow(float3(0.1f, 0.1f, 0.1f), 1.0f / gamma), (rad.r - rad2.r) / rad2.r);
        //col = float3(d.x, d.y, 0.0f);
        //rad = (clamp(float3(1.0f, 1.0f, 1.0f) - rad, float3(0.5f, 0.5f, 0.5f), float3(1.0f, 1.0f, 1.0f)) - 0.5f) * 2.0f;
        //rad2 = (clamp(float3(1.0f, 1.0f, 1.0f) - rad2, float3(0.5f, 0.5f, 0.5f), float3(1.0f, 1.0f, 1.0f)) - 0.5f) * 2.0f;
        //contrast = ((float3(1.0f, 1.0f, 1.0f) - rad.r) - (float3(1.0f, 1.0f, 1.0f) - rad2.r)) / (float3(1.0f, 1.0f, 1.0f) - rad2.r);
        //col = rad; //(clamp(float3(1.0f, 1.0f, 1.0f) - rad, float3(0.8f, 0.8f, 0.8f), float3(1.0f, 1.0f, 1.0f)) - 0.8f) * 5.0f;
        contrast = max(pow(float3(0.0f, 0.0f, 0.0f), 1.0f / gamma), abs(rad.r - rad2.r) / rad2.r);
        //col = rad;
        col = saturate( /*pow(specularIrrad.Sample(texSampler, uvCoord.xy).rgb, 1.0f / gamma) + */sss);
    }

 //(maxRad - minRad) / (maxRad+minRad);
    
    contrast = ((clamp(contrast, 0.5f, 1.0f) - 0.5f) * 2) > 0.2f ? 1.0f : 0.0f;
    

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
    //            float3 rad = pow(diffuseIrrad.Sample(texSampler, coords).rgb, 1.0f / gamma) + pow(transmittedIrrad.Sample(texSampler, coords).rgb, 1.0f / gamma);
    //            maxRad = max(maxRad, rad.r);
    //            minRad = min(minRad, rad.r);
    //            col += (gaussian[i] * (rad * weights[j])) * 0.5f;
    //            coords += float2(netFilterWidth, 0.0f);
    //        }
        
    //        netFilterWidth = (1 / 720.0f) * gaussWidths[j] * sy;
    //        coords = uvCoord - float2(0.0f, netFilterWidth * 3.0);
    //        for (int i2 = 0; i2 < 7; i2++)
    //        {
    //            float3 rad = pow(diffuseIrrad.Sample(texSampler, coords).rgb, 1.0f / gamma) + pow(transmittedIrrad.Sample(texSampler, coords).rgb, 1.0f / gamma);
    //            maxRad = max(maxRad, rad.r);
    //            minRad = min(minRad, rad.r);
    //            col += (gaussian[i2] * rad * weights[j]) * 0.5f;
    //            coords += float2(0.0f, netFilterWidth);
    //        }
    //    }
    //}
    
    
    
    
    //col = saturate(pow(specularIrrad.Sample(texSampler, uvCoord.xy).rgb, 1.0f/gamma) + col);
    //contrast = ddx(contrast);
    //col = float3(contrast, contrast, contrast);
    //float3 col2 = saturate(float3(contrast2, contrast2, contrast2));
    //col = abs(float3(t, t, t));
    //col = rad2;
    //col = contrast < 0.2f ? float3(0.1f, 0.1f, 0.1f) : float3(1.0f, 1.0f, 1.0f);
    return float4(pow(col /* + pow(diffuseIrrad.SampleLevel(texSampler, uvCoord, 0.0f).rgb, 1.0f / gamma) * 0.8f*/, gamma), 1.0f);
    //return float4(col, 1.0f);
    
    }