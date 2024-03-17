#include "RendererBackend.h"
#include "RendererFrontend.h"
#include "RenderHardwareInterface.h"
#include "RHIState.h"
#include "BufferCache.h"
#include "ResourceCache.h"
#include "Material.h"
#include "Lights.h"
#include "Camera.h"
#include "RenderDevice.h"
#include "ApplicationConfig.h"

#include <DDSTextureLoader.h>

#include <d3d11_1.h>
#include <vector>
#include <cassert>
#include <fstream>
#include <iostream>


RendererBackend rb;

RendererBackend::RendererBackend()
{
    sm_viewport = D3D11_VIEWPORT{};
    sceneViewport = D3D11_VIEWPORT{};

	matPrmsCB = nullptr;
    lightsCB = nullptr;
    objectCB = nullptr;
    projCB = nullptr;
    viewCB = nullptr;
    dirShadCB = nullptr;
    omniDirShadCB = nullptr;

    shadowMap = nullptr;
    shadowMapDSV = nullptr;
    shadowMapSRV = nullptr;

    cubemapView = nullptr;
    
    shadowCubeMap = nullptr;
    shadowCubeMapSRV = nullptr;
    for (int i = 0; i < 6; i++) {
        shadowCubeMapDSV[i] = nullptr;
    }

    for (int i = 0; i < 5; i++) {
        skinRT[i] = nullptr;
        skinMaps[i] = nullptr;
        skinSRV[i] = nullptr;
    }

    for (int i = 0; i < GBuffer::SIZE; i++) {
        GBufferSRV[i] = nullptr;
        GBufferRTV[i] = nullptr;
        GBuffer[i] = nullptr;
    }

    invMatCB = nullptr;

    annot = nullptr;
}


RendererBackend::~RendererBackend()
{
}

void RendererBackend::ReleaseAllResources() {
    if (matPrmsCB) matPrmsCB->Release();
    if (lightsCB) lightsCB->Release();
    if (objectCB) objectCB->Release();
    if (viewCB) viewCB->Release();
    if (projCB) projCB->Release();
    if (dirShadCB) dirShadCB->Release();
    if (omniDirShadCB) omniDirShadCB->Release();

    if (invMatCB) invMatCB->Release();
    if (annot) annot->Release();

    if (thicknessMapSRV.resourceView) thicknessMapSRV.resourceView->Release();
    if (cubemapView) cubemapView->Release();
}

struct KernelSample
{
    float r, g, b, x, y;
};

float nSamples = 7;
std::vector<dx::XMFLOAT4> kernel;

//void loadKernelFile(std::string fileName, std::vector<float>& data)
//{
//    //data.clear();
//
//    //std::string folder(kernelFolder.begin(), kernelFolder.end()); // dirty conversion
//    std::string path = fileName;//folder + fileName;
//
//    bool binary = false;
//    if (fileName.compare(fileName.size() - 3, 3, ".bn") == 0)
//        binary = true;
//
//    std::ifstream i;
//    std::ios_base::openmode om;
//
//    if (binary) om = std::ios_base::in | std::ios_base::binary;
//    else om = std::ios_base::in;
//
//    i.open(path, om);
//
//    if (!i.good())
//    {
//        i.close();
//        i.open("../" + path, om);
//    }
//
//    if (binary)
//    {
//        data.clear();
//
//        // read float count
//        char sv[4];
//        i.read(sv, 4);
//        int fc = (int)(floor(*((float*)sv)));
//
//        data.resize(fc);
//        i.read(reinterpret_cast<char*>(&data[0]), fc * 4);
//    }
//    else
//    {
//        float v;
//
//        while (i >> v)
//        {
//            data.push_back(v);
//
//            int next = i.peek();
//            switch (next)
//            {
//                case ',': i.ignore(1); break;
//                case ' ': i.ignore(1); break;
//            }
//        }
//    }
//
//    i.close();
//}
//
//void calculateOffsets(float _range, float _exponent, int _offsetCount, std::vector<float>& _offsets)
//{
//    // Calculate the offsets:
//    float step = 2.0f * _range / (_offsetCount - 1);
//    for (int i = 0; i < _offsetCount; i++) {
//        float o = -_range + float(i) * step;
//        float sign = o < 0.0f ? -1.0f : 1.0f;
//        float ofs = _range * sign * abs(pow(o, _exponent)) / pow(_range, _exponent);
//        _offsets.push_back(ofs);
//    }
//}
//
//void calculateAreas(std::vector<float>& _offsets, std::vector<float>& _areas)
//{
//    int size = _offsets.size();
//
//    for (int i = 0; i < size; i++) {
//        float w0 = i > 0 ? abs(_offsets[i] - _offsets[i - 1]) : 0.0f;
//        float w1 = i < size - 1 ? abs(_offsets[i] - _offsets[i + 1]) : 0.0f;
//        float area = (w0 + w1) / 2.0f;
//        _areas.push_back(area);
//    }
//}
//
//dx::XMFLOAT3 linInterpol1D(std::vector<KernelSample> _kernelData, float _x)
//{
//    // naive, a lot to improve here
//
//    if (_kernelData.size() < 1) throw "_kernelData empty";
//
//    unsigned int i = 0;
//    while (i < _kernelData.size())
//    {
//        if (_x > _kernelData[i].x) i++;
//        else break;
//    }
//
//    dx::XMFLOAT3 v;
//
//    if (i < 1)
//    {
//        v.x = _kernelData[0].r;
//        v.y = _kernelData[0].g;
//        v.z = _kernelData[0].b;
//    }
//    else if (i > _kernelData.size() - 1)
//    {
//        v.x = _kernelData[_kernelData.size() - 1].r;
//        v.y = _kernelData[_kernelData.size() - 1].g;
//        v.z = _kernelData[_kernelData.size() - 1].b;
//    }
//    else
//    {
//        KernelSample b = _kernelData[i];
//        KernelSample a = _kernelData[i - 1];
//
//        float d = b.x - a.x;
//        float dx = _x - a.x;
//
//        float t = dx / d;
//
//        v.x = a.r * (1 - t) + b.r * t;
//        v.y = a.g * (1 - t) + b.g * t;
//        v.z = a.b * (1 - t) + b.b * t;
//    }
//
//    return v;
//}
//
//void calculateSsssDiscrSepKernel(const std::vector<KernelSample>& _kernelData)
//{
//    const float EXPONENT = 2.0f; // used for impartance sampling
//
//    float RANGE = _kernelData[_kernelData.size() - 1].x; // get max. sample location
//
//    // calculate offsets
//    std::vector<float> offsets;
//    calculateOffsets(RANGE, EXPONENT, nSamples, offsets);
//
//    // calculate areas (using importance-sampling) 
//    std::vector<float> areas;
//    calculateAreas(offsets, areas);
//
//    kernel.resize(nSamples);
//
//    dx::XMFLOAT3 sum = dx::XMFLOAT3(0, 0, 0); // weights sum for normalization
//
//    // compute interpolated weights
//    for (int i = 0; i < nSamples; i++)
//    {
//        float sx = offsets[i];
//
//        dx::XMFLOAT3 v = linInterpol1D(_kernelData, sx);
//        kernel[i].x = v.x * areas[i];
//        kernel[i].y = v.y * areas[i];
//        kernel[i].z = v.z * areas[i];
//        kernel[i].w = sx;
//
//        sum.x += kernel[i].x;
//        sum.y += kernel[i].y;
//        sum.z += kernel[i].z;
//    }
//
//    // Normalize
//    for (int i = 0; i < nSamples; i++) {
//        kernel[i].x /= sum.x;
//        kernel[i].y /= sum.y;
//        kernel[i].z /= sum.z;
//    }
//
//    // TEMP put center at first
//    dx::XMFLOAT4 t = kernel[nSamples / 2];
//    for (int i = nSamples / 2; i > 0; i--)
//        kernel[i] = kernel[i - 1];
//    kernel[0] = t;
//
//    // set shader vars
//    //HRESULT hr;
//    //V(effect->GetVariableByName("maxOffsetMm")->AsScalar()->SetFloat(RANGE));
//    //V(kernelVariable->SetFloatVectorArray((float*)&kernel.front(), 0, nSamples));
//}
//
//void overrideSsssDiscrSepKernel(const std::vector<float>& _kernelData)
//{
//    bool useImg2DKernel = false;
//
//    // conversion of linear kernel data to sample array
//    std::vector<KernelSample> k;
//    unsigned int size = _kernelData.size() / 4;
//
//    unsigned int i = 0;
//    for (unsigned int s = 0; s < size; s++)
//    {
//        KernelSample ks;
//        ks.r = _kernelData[i++];
//        ks.g = _kernelData[i++];
//        ks.b = _kernelData[i++];
//        ks.x = _kernelData[i++];
//        k.push_back(ks);
//    }
//
//    // kernel computation
//    calculateSsssDiscrSepKernel(k);
//}
//
//dx::XMFLOAT3 gauss1D(float x, dx::XMFLOAT3 variance)
//{
//    dx::XMVECTOR var = dx::XMLoadFloat3(&variance);
//    dx::XMVECTOR var2 = dx::XMVectorMultiply(var, var);
//    dx::XMVECTOR var2_2 = dx::XMVectorAdd(var2, var2);
//    dx::XMVECTOR xVec = dx::XMVectorMultiply(dx::XMVectorSet(x, x, x, 0.0f), dx::XMVectorSet(x, x, x, 0.0f));
//    dx::XMVECTOR negXVec = dx::XMVectorNegate(xVec);
//    
//    dx::XMVECTOR res = dx::XMVectorMultiply(dx::XMVectorMultiply(dx::XMVectorReciprocal(var), dx::XMVectorReciprocalSqrt(dx::g_XMPi+ dx::g_XMPi)),dx::XMVectorExp(dx::XMVectorMultiply(negXVec,dx::XMVectorReciprocal(var2_2))));
//    return dx::XMFLOAT3(res.m128_f32[0], res.m128_f32[1], res.m128_f32[2]);
//    //return rcp(sqrt(2.0f * dx::XM_PI) * variance) * exp(-(x * x) * rcp(2.0f * variance * variance));
//}
//
//dx::XMFLOAT3 profile(float r) {
//    /**
//     * We used the red channel of the original skin profile defined in
//     * [d'Eon07] for all three channels. We noticed it can be used for green
//     * and blue channels (scaled using the falloff parameter) without
//     * introducing noticeable differences and allowing for total control over
//     * the profile. For example, it allows to create blue SSS gradients, which
//     * could be useful in case of rendering blue creatures.
//     */
//    //return  // 0.233f * gaussian(0.0064f, r) + /* We consider this one to be directly bounced light, accounted by the strength parameter (see @STRENGTH) */
//  /*      0.100f * gaussian(0.0484f, r) +
//        0.118f * gaussian(0.187f, r) +
//        0.113f * gaussian(0.567f, r) +
//        0.358f * gaussian(1.99f, r) +
//        0.078f * gaussian(7.41f, r);*/
//    dx::XMFLOAT3 nearVar = dx::XMFLOAT3(0.077f, 0.034f, 0.02f);
//    dx::XMFLOAT3 farVar = dx::XMFLOAT3(1.0f, 0.45f, 0.25f);
//    dx::XMVECTOR g1 = dx::XMLoadFloat3(&(gauss1D(r, nearVar)));
//    dx::XMVECTOR g2 = dx::XMLoadFloat3(&(gauss1D(r, farVar)));
//    dx::XMVECTOR weight = dx::XMVectorSet(0.5, 0.5f, 0.5f, 0.0f);
//    dx::XMVECTOR res = dx::XMVectorAdd(dx::XMVectorMultiply(weight, g1), dx::XMVectorMultiply(weight, g2));
//
//    return dx::XMFLOAT3(res.m128_f32[0], res.m128_f32[1], res.m128_f32[2]);
//}
//
//void SeparableSSScalculateKernel() {
//    HRESULT hr;
//
//    const float RANGE = nSamples > 20 ? 3.0f : 2.0f;
//    const float EXPONENT = 2.0f;
//
//    kernel.resize(nSamples);
//
//    // Calculate the offsets:
//    float step = 2.0f * RANGE / (nSamples - 1);
//    for (int i = 0; i < nSamples; i++) {
//        float o = -RANGE + float(i) * step;
//        float sign = o < 0.0f ? -1.0f : 1.0f;
//        kernel[i].w = RANGE * sign * abs(pow(o, EXPONENT)) / pow(RANGE, EXPONENT);
//    }
//
//    // Calculate the weights:
//    for (int i = 0; i < nSamples; i++) {
//        float w0 = i > 0 ? abs(kernel[i].w - kernel[i - 1].w) : 0.0f;
//        float w1 = i < nSamples - 1 ? abs(kernel[i].w - kernel[i + 1].w) : 0.0f;
//        float area = (w0 + w1) / 2.0f;
//        dx::XMFLOAT3 prof = profile(kernel[i].w);
//        dx::XMFLOAT3 t = dx::XMFLOAT3(area * prof.x, area * prof.y, area * prof.z);
//        kernel[i].x = t.x;
//        kernel[i].y = t.y;
//        kernel[i].z = t.z;
//    }
//
//    // We want the offset 0.0 to come first:
//    dx::XMFLOAT4 t = kernel[nSamples / 2];
//    for (int i = nSamples / 2; i > 0; i--)
//        kernel[i] = kernel[i - 1];
//    kernel[0] = t;
//
//    // Calculate the sum of the weights, we will need to normalize them below:
//    dx::XMFLOAT3 sum = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
//    for (int i = 0; i < nSamples; i++) {
//        sum = dx::XMFLOAT3(kernel[i].x+sum.x, kernel[i].y + sum.y, kernel[i].z+sum.z);
//    }
//
//    // Normalize the weights:
//    for (int i = 0; i < nSamples; i++) {
//        kernel[i].x /= sum.x;
//        kernel[i].y /= sum.y;
//        kernel[i].z /= sum.z;
//    }
//
//    // Tweak them using the desired strength. The first one is:
//    //     lerp(1.0, kernel[0].rgb, strength)
//    //kernel[0].x = (1.0f - strength.x) * 1.0f + strength.x * kernel[0].x;
//    //kernel[0].y = (1.0f - strength.y) * 1.0f + strength.y * kernel[0].y;
//    //kernel[0].z = (1.0f - strength.z) * 1.0f + strength.z * kernel[0].z;
//
//    //// The others:
//    ////     lerp(0.0, kernel[0].rgb, strength)
//    //for (int i = 1; i < nSamples; i++) {
//    //    kernel[i].x *= strength.x;
//    //    kernel[i].y *= strength.y;
//    //    kernel[i].z *= strength.z;
//    //}
//
//}


HRESULT RendererBackend::Init(bool reset)
{
    if (reset) rhi.device.releaseAllResources();

    rhi.context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&annot);

    sceneViewport.TopLeftX = 0.0f;
    sceneViewport.TopLeftY = 0.0f;
    sceneViewport.Width = g_resolutionWidth;
    sceneViewport.Height = g_resolutionHeight;
    sceneViewport.MinDepth = 0.0f;
    sceneViewport.MaxDepth = 1.0f;

    sm_viewport.TopLeftX = 0.0f;
    sm_viewport.TopLeftY = 0.0f;
    sm_viewport.Width = 2048.0f;
    sm_viewport.Height = 2048.f;
    sm_viewport.MinDepth = 0.0f;
    sm_viewport.MaxDepth = 1.0f;

	D3D11_BUFFER_DESC matCbDesc;
	ZeroMemory(&matCbDesc, sizeof(matCbDesc));
	matCbDesc.ByteWidth = sizeof(MaterialParams);
	matCbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matCbDesc.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr = rhi.device.device->CreateBuffer(&matCbDesc, nullptr, &matPrmsCB);
    
    RETURN_IF_FAILED(hr);

    //World transform constant buffer setup---------------------------------------------------
    D3D11_BUFFER_DESC cbObjDesc;
    cbObjDesc.Usage = D3D11_USAGE_DEFAULT;
    cbObjDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbObjDesc.CPUAccessFlags = 0;
    cbObjDesc.MiscFlags = 0;
    cbObjDesc.ByteWidth = sizeof(ObjCB);

    hr = rhi.device.device->CreateBuffer(&cbObjDesc, nullptr, &objectCB);
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
    //---------------------------------------------------


    //View matrix constant buffer setup-------------------------------------------------------------
    D3D11_BUFFER_DESC cbCamDesc;
    cbCamDesc.Usage = D3D11_USAGE_DEFAULT;
    cbCamDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbCamDesc.CPUAccessFlags = 0;
    cbCamDesc.MiscFlags = 0;
    cbCamDesc.ByteWidth = sizeof(ViewCB);

    hr = rhi.device.device->CreateBuffer(&cbCamDesc, nullptr, &viewCB);
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
    //----------------------------------------------------------------


    //Projection matrix constant buffer setup--------------------------------------
    D3D11_BUFFER_DESC cbProjDesc;
    cbProjDesc.Usage = D3D11_USAGE_DEFAULT;
    cbProjDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbProjDesc.CPUAccessFlags = 0;
    cbProjDesc.MiscFlags = 0;
    cbProjDesc.ByteWidth = sizeof(ProjCB);

    hr = rhi.device.device->CreateBuffer(&cbProjDesc, nullptr, &projCB);
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(2, 1, &projCB);
    //------------------------------------------------------------------

    //Light constant buffer setup---------------------------------------
    D3D11_BUFFER_DESC lightBuffDesc;
    ZeroMemory(&lightBuffDesc, sizeof(lightBuffDesc));
    lightBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    lightBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightBuffDesc.ByteWidth = sizeof(LightCB);

    hr = rhi.device.device->CreateBuffer(&lightBuffDesc, nullptr, &lightsCB);
    RETURN_IF_FAILED(hr);

    rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
    rhi.context->VSSetConstantBuffers(3, 1, &lightsCB);
    //---------------------------------------------------------
	
    //GBuffer setup--------------------------------------------
    
    ZRYResult zr;

    RenderTextureHandle gBuffHnd[GBuffer::SIZE];
    zr = rhi.device.createTex2D(&gBuffHnd[0], ZRYBindFlags{ D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE }, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, GBuffer::SIZE, nullptr, nullptr, true, 4, 1);
    RETURN_IF_FAILED(zr.value);
    for (int i = 0; i < GBuffer::SIZE; i++) {
        GBuffer[i] = rhi.device.getTex2DPointer(gBuffHnd[i]);
    }

    RenderSRVHandle gBuffSrvHnd[GBuffer::SIZE];
    zr = rhi.device.createSRVTex2D(gBuffSrvHnd, gBuffHnd, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_UNORM }, GBuffer::SIZE, 4);
    RETURN_IF_FAILED(zr.value);
    for (int i = 0; i < GBuffer::SIZE; i++) {
        GBufferSRV[i] = rhi.device.getSRVPointer(gBuffSrvHnd[i]);
    }

    RenderRTVHandle gBuffRtvHnd[GBuffer::SIZE];
    zr = rhi.device.createRTVTex2D(gBuffRtvHnd, gBuffHnd, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_UNORM }, GBuffer::SIZE);
    RETURN_IF_FAILED(zr.value);
    for (int i = 0; i < GBuffer::SIZE; i++) {
        GBufferRTV[i] = rhi.device.getRTVPointer(gBuffRtvHnd[i]);
    }

    //----------------------------------------------------------

    //ambient setup------------------------------------------------
    RenderTextureHandle ambientHnd;
    zr = rhi.device.createTex2D(&ambientHnd, ZRYBindFlags{ D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE }, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, 1, nullptr, nullptr, true, 0, 1);
    RETURN_IF_FAILED(zr.value);
    ambientMap = rhi.device.getTex2DPointer(ambientHnd);

    RenderSRVHandle ambientSrvHnd;
    zr = rhi.device.createSRVTex2D(&ambientSrvHnd, &ambientHnd, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_UNORM });
    RETURN_IF_FAILED(zr.value);
    ambientSRV = rhi.device.getSRVPointer(ambientSrvHnd);

    RenderRTVHandle ambientRtvHnd;
    zr = rhi.device.createRTVTex2D(&ambientRtvHnd, &ambientHnd, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_UNORM }, 1);
    RETURN_IF_FAILED(zr.value);
    ambientRTV = rhi.device.getRTVPointer(ambientRtvHnd);
    //-----------------------------------------------------------


    D3D11_BUFFER_DESC invMatDesc;
    ZeroMemory(&invMatDesc, sizeof(invMatDesc));
    invMatDesc.ByteWidth = sizeof(dx::XMMatrixIdentity()) * 2;
    invMatDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    invMatDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = rhi.device.device->CreateBuffer(&invMatDesc, nullptr, &invMatCB);

    //---------------------------------------------------------

    //Shadow map setup-------------------------------------------------
    D3D11_BUFFER_DESC dirShadowMatBuffDesc;
    ZeroMemory(&dirShadowMatBuffDesc, sizeof(D3D11_BUFFER_DESC));
    dirShadowMatBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    dirShadowMatBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    dirShadowMatBuffDesc.ByteWidth = sizeof(DirShadowCB);

    hr = rhi.device.device->CreateBuffer(&dirShadowMatBuffDesc, nullptr, &dirShadCB);
    RETURN_IF_FAILED(hr);

    D3D11_BUFFER_DESC omniDirShadowMatBuffDesc;
    ZeroMemory(&omniDirShadowMatBuffDesc, sizeof(D3D11_BUFFER_DESC));
    omniDirShadowMatBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    omniDirShadowMatBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    omniDirShadowMatBuffDesc.ByteWidth = sizeof(OmniDirShadowCB);

    hr = rhi.device.device->CreateBuffer(&omniDirShadowMatBuffDesc, nullptr, &omniDirShadCB);
    RETURN_IF_FAILED(hr);
    

    RenderTextureHandle shadowMapHnd;
    zr = rhi.device.createTex2D(&shadowMapHnd, ZRYBindFlags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL }, ZRYFormat{ DXGI_FORMAT_R24G8_TYPELESS }, sm_viewport.Width, sm_viewport.Height, 1, nullptr, nullptr, false, 1);
    RETURN_IF_FAILED(zr.value);
    shadowMap = rhi.device.getTex2DPointer(shadowMapHnd);

    RenderSRVHandle shadowMapSrvHnd;
    zr = rhi.device.createSRVTex2D(&shadowMapSrvHnd, &shadowMapHnd, ZRYFormat{ DXGI_FORMAT_R24_UNORM_X8_TYPELESS }, 1, -1);
    RETURN_IF_FAILED(zr.value);
    shadowMapSRV = rhi.device.getSRVPointer(shadowMapSrvHnd);

    RenderDSVHandle hndDSVShadowMap;
    zr = rhi.device.createDSVTex2D(&hndDSVShadowMap, &shadowMapHnd, ZRYFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT });
    RETURN_IF_FAILED(zr.value);
    shadowMapDSV = rhi.device.getDSVPointer(hndDSVShadowMap);


    //Shadow map setup (spot light)-----------------------------------
    RenderTextureHandle spotShadowMapHnd;
    zr = rhi.device.createTex2D(&spotShadowMapHnd, ZRYBindFlags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL }, ZRYFormat{ DXGI_FORMAT_R24G8_TYPELESS }, sm_viewport.Width, sm_viewport.Height, 1, nullptr, nullptr, false, 1);
    RETURN_IF_FAILED(zr.value);
    spotShadowMap = rhi.device.getTex2DPointer(spotShadowMapHnd);

    RenderSRVHandle spotShadowMapSrvHnd;
    zr = rhi.device.createSRVTex2D(&spotShadowMapSrvHnd, &spotShadowMapHnd, ZRYFormat{ DXGI_FORMAT_R24_UNORM_X8_TYPELESS }, 1, -1);
    RETURN_IF_FAILED(zr.value);
    spotShadowMapSRV = rhi.device.getSRVPointer(spotShadowMapSrvHnd);

    RenderDSVHandle hndDsvSpotShadowMap;
    zr = rhi.device.createDSVTex2D(&hndDsvSpotShadowMap, &spotShadowMapHnd, ZRYFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT });
    RETURN_IF_FAILED(zr.value);
    spotShadowMapDSV = rhi.device.getDSVPointer(hndDsvSpotShadowMap);


    //Cube shadow map setup (point light)--------------------------------------------
    RenderTextureHandle shadowCubemapHnd;
    zr = rhi.device.createTexCubemap(&shadowCubemapHnd, ZRYBindFlags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL }, ZRYFormat{ DXGI_FORMAT_R24G8_TYPELESS },
        sm_viewport.Width, sm_viewport.Height, 1, nullptr, nullptr, false, 1, 12);
    RETURN_IF_FAILED(zr.value);
    shadowCubeMap = rhi.device.getTex2DPointer(shadowCubemapHnd);

    RenderSRVHandle hndShadowCubemapSRV;
    zr = rhi.device.createSRVTex2DArray(&hndShadowCubemapSRV, &shadowCubemapHnd, ZRYFormat{ DXGI_FORMAT_R24_UNORM_X8_TYPELESS }, 1, 12);
    RETURN_IF_FAILED(zr.value);
    shadowCubeMapSRV = rhi.device.getSRVPointer(hndShadowCubemapSRV);

    int numScmDSV = ARRAYSIZE(shadowCubeMapDSV);
    RenderDSVHandle* hndDSVShadowCubemap = new RenderDSVHandle[numScmDSV];
    
    for (int i = 0; i < numScmDSV; i++)
    {
        zr = rhi.device.createDSVTex2DArray(&hndDSVShadowCubemap[i], &shadowCubemapHnd, ZRYFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT }, 1, 1, 0, i);
        RETURN_IF_FAILED(zr.value);
        shadowCubeMapDSV[i] = rhi.device.getDSVPointer(hndDSVShadowCubemap[i]);
    }
    
    //-----------------------------------------------------------------

    //Irradiance map setup---------------------------------------------

    RenderTextureHandle skinMapHnd[5];
    RenderRTVHandle skinRTViewHnd[5];
    RenderSRVHandle skinSRViewHnd[5];
    zr = rhi.device.createTex2D(&skinMapHnd[0], ZRYBindFlags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET }, ZRYFormat{ DXGI_FORMAT_R32G32B32A32_FLOAT }, g_resolutionWidth, g_resolutionHeight, 3, &skinSRViewHnd[0], &skinRTViewHnd[0]);
    RETURN_IF_FAILED(zr.value);

    zr = rhi.device.createTex2D(&skinMapHnd[4], ZRYBindFlags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET }, ZRYFormat{ DXGI_FORMAT_R32G32B32A32_FLOAT }, g_resolutionWidth, g_resolutionHeight, 1, &skinSRViewHnd[4], &skinRTViewHnd[4]);
    RETURN_IF_FAILED(zr.value);

    zr = rhi.device.createTex2D(&skinMapHnd[3], ZRYBindFlags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET }, ZRYFormat{ DXGI_FORMAT_R32G32B32A32_FLOAT }, g_resolutionWidth, g_resolutionHeight, 1, &skinSRViewHnd[3], &skinRTViewHnd[3], true, 8);
    RETURN_IF_FAILED(zr.value);

    for (int i = 0; i < 5; i++) {
        skinMaps[i] = rhi.device.getTex2DPointer(skinMapHnd[i]);
        skinRT[i] = rhi.device.getRTVPointer(skinRTViewHnd[i]);
        skinSRV[i] = rhi.device.getSRVPointer(skinSRViewHnd[i]);
    }

    //-----------------------------------------------------------------

    RenderTextureHandle hndFinalRT;
    zr = rhi.device.createTex2D(&hndFinalRT, ZRYBindFlags{ D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE }, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, 1, nullptr, nullptr, false, 1, 1);
    RETURN_IF_FAILED(zr.value);
    finalRenderTargetTex = rhi.device.getTex2DPointer(hndFinalRT);

    RenderSRVHandle hndFinalSRV;
    zr = rhi.device.createSRVTex2D(&hndFinalSRV, &hndFinalRT, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_UNORM });
    RETURN_IF_FAILED(zr.value);
    finalRenderTargetSRV = rhi.device.getSRVPointer(hndFinalSRV);

    RenderRTVHandle hndFinalRTV;
    zr = rhi.device.createRTVTex2D(&hndFinalRTV, &hndFinalRT, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_UNORM }, 1);
    RETURN_IF_FAILED(zr.value);
    finalRenderTargetView = rhi.device.getRTVPointer(hndFinalRTV);

    //Depth stencil------------------------
    RenderTextureHandle hndDepthTex;
    zr = rhi.device.createTex2D(&hndDepthTex, ZRYBindFlags{ D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE }, ZRYFormat{ DXGI_FORMAT_R24G8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, 1, nullptr, nullptr, false, 1, 1);
    RETURN_IF_FAILED(zr.value);
    depthTex = rhi.device.getTex2DPointer(hndDepthTex);

    RenderSRVHandle hndDepthSRV;
    zr = rhi.device.createSRVTex2D(&hndDepthSRV, &hndDepthTex, ZRYFormat{ DXGI_FORMAT_R24_UNORM_X8_TYPELESS });
    RETURN_IF_FAILED(zr.value);
    depthSRV = rhi.device.getSRVPointer(hndDepthSRV);

    RenderDSVHandle hndDepthDSV;
    zr = rhi.device.createDSVTex2D(&hndDepthDSV, &hndDepthTex, ZRYFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT }, 1);
    RETURN_IF_FAILED(zr.value);
    depthDSV = rhi.device.getDSVPointer(hndDepthDSV);


    //----------------------------Skybox----------------------
    wrl::ComPtr<ID3D11Resource> skyTexture;
    hr = dx::CreateDDSTextureFromFileEx(rhi.device.device, L"./shaders/assets/skybox.dds", 0, D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, dx::DX11::DDS_LOADER_DEFAULT, skyTexture.GetAddressOf(), &cubemapView);
    RETURN_IF_FAILED(hr);

    return hr;
    //---------------------------------------------------------------------

    //thickness map
    rhi.LoadTexture(L"./shaders/assets/Human/Textures/Head/JPG/baked_translucency_4096.jpg", thicknessMapSRV, false);
    
    std::vector<float> krn;
    ////loadKernelFile("./shaders/assets/Skin2_PreInt_DISCSEP.bn", krn);
    ////loadKernelFile("./shaders/assets/Skin1_PreInt_DISCSEP.bn", krn);
    //loadKernelFile("./shaders/assets/Skin1_ArtModProd_DISCSEP.bn", krn);
    //
    //overrideSsssDiscrSepKernel(krn);
    ////SeparableSSScalculateKernel();
    //for (int i = 2; i < kernel.size(); i++) {
    //    OutputDebugString(std::to_string(abs(kernel[i].w - kernel[i - 1].w)).c_str());
    //    OutputDebugString("\n");
    //}

    return S_OK;
}

void RendererBackend::RenderShadowMaps(const ViewDesc& viewDesc, DirShadowCB& dirShadowCB, OmniDirShadowCB& cbOmniDirShad ) {
    annot->BeginEvent(L"ShadowMap Pre-Pass");
    {
        std::vector<LightInfo> dirLights;
        std::vector<LightInfo> spotLights;
        std::vector<LightInfo> pointLights;

        ViewCB tmpVCB;
        ProjCB tmpPCB;
        ObjCB tmpOCB;
        std::uint32_t strides[] = { sizeof(Vertex) };
        std::uint32_t offsets[] = { 0 };
        

        rhi.context->RSSetViewports(1, &sm_viewport);

        for (const LightInfo& lightInfo : viewDesc.lightsInfo) {
            switch (lightInfo.tag)
            {
                case LightType::DIRECTIONAL:
                {
                    dirLights.push_back(lightInfo);
                    break;
                }

                case LightType::SPOT:
                {
                    spotLights.push_back(lightInfo);
                    break;
                }

                case LightType::POINT:
                {
                    pointLights.push_back(lightInfo);
                    break;
                }
            }
        }

        int numSpotLights = spotLights.size();
        assert(numSpotLights == viewDesc.numSpotLights);

        int numPointLights = pointLights.size();
        assert(numPointLights == viewDesc.numPointLights);

        int numDirLigths = dirLights.size();
        assert(numDirLigths == viewDesc.numDirLights);

        rhi.context->PSSetShader(nullptr, nullptr, 0);
        rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::DEPTH), nullptr, 0);

        //Directional lights--------------------------------------------
        annot->BeginEvent(L"Directional Lights");

        rhi.context->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        rhi.context->OMSetRenderTargets(0, nullptr, shadowMapDSV);

        rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
        rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
        rhi.context->VSSetConstantBuffers(2, 1, &projCB);

        for (int i = 0; i < numDirLigths; i++)
        {
            DirectionalLight& dirLight = dirLights.at(i).dirLight;
            dx::XMVECTOR transfDirLight = dx::XMVector4Transform(dirLight.direction, dirLights.at(i).finalWorldTransf);
            dx::XMVECTOR lightPos = dx::XMVectorMultiply(dx::XMVectorNegate(transfDirLight), dx::XMVectorSet(3.0f, 3.0f, 3.0f, 1.0f));
            //TODO: rename these matrices; it isnt clear that they are used for shadow mapping
            dx::XMMATRIX dirLightVMat = dx::XMMatrixLookAtLH(lightPos, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
            dx::XMMATRIX dirLightPMat = dx::XMMatrixOrthographicLH(4.0f, 4.0f, dirLight.shadowMapNearPlane, dirLight.shadowMapFarPlane);

            dirShadowCB.dirPMat = dx::XMMatrixTranspose(dirLightPMat);
            dirShadowCB.dirVMat = dx::XMMatrixTranspose(dirLightVMat);
            cbOmniDirShad.dirLightProjMat = dirShadowCB.dirPMat;
            cbOmniDirShad.dirLightViewMat = dirShadowCB.dirVMat;

            tmpVCB.viewMatrix = dx::XMMatrixTranspose(dirLightVMat);
            tmpPCB.projMatrix = dx::XMMatrixTranspose(dirLightPMat);

            rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
            rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

            for (SubmeshInfo const& sbPair : viewDesc.submeshesInfo) {
                rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
                rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

                tmpOCB.worldMatrix = dx::XMMatrixTranspose(sbPair.finalWorldTransf);
                rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

                RHIState state = RHI_DEFAULT_STATE();
                RHI_RS_SET_CULL_BACK(state);
                RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

                rhi.SetState(state);

                rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
            }
        }
        annot->EndEvent();

        //-------------------------------------------------------------------------------

        //Spot lights--------------------------------------------------------------------
        annot->BeginEvent(L"Spot Lights");

        rhi.context->OMSetRenderTargets(0, nullptr, spotShadowMapDSV);
        rhi.context->ClearDepthStencilView(spotShadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        for (int i = 0; i < numSpotLights; i++) {
            dx::XMVECTOR finalPos = dx::XMVector4Transform(spotLights[i].spotLight.posWorldSpace, spotLights[i].finalWorldTransf);
            dx::XMVECTOR finalDir = dx::XMVector4Transform(spotLights[i].spotLight.direction, spotLights[i].finalWorldTransf);
            cbOmniDirShad.spotLightProjMat[i] = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(std::acos(spotLights[i].spotLight.cosCutoffAngle) * 2.0f, 1.0f, spotLights[i].spotLight.shadowMapNearPlane, spotLights[i].spotLight.shadowMapFarPlane));  //multiply acos by 2, because cutoff angle is considered from center, not entire light angle
            cbOmniDirShad.spotLightViewMat[i] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, finalDir, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

            tmpVCB.viewMatrix = cbOmniDirShad.spotLightViewMat[i];
            tmpPCB.projMatrix = cbOmniDirShad.spotLightProjMat[i];

            rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
            rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

            for (SubmeshInfo const& sbPair : viewDesc.submeshesInfo) {
                rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
                rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

                tmpOCB.worldMatrix = dx::XMMatrixTranspose(sbPair.finalWorldTransf);
                rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

                RHIState state = RHI_DEFAULT_STATE();
                RHI_RS_SET_CULL_BACK(state);
                RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

                rhi.SetState(state);

                rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
            }
        }
        annot->EndEvent();

        //Point lights-------------------------------------------------------------------
        //TODO: do something about this; shouldnt hard code index in pointLights
        annot->BeginEvent(L"Point Lights");

        if (numPointLights > 0) cbOmniDirShad.pointLightProjMat = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(dx::XM_PIDIV2, 1.0f, pointLights[0].pointLight.shadowMapNearPlane, pointLights[0].pointLight.shadowMapFarPlane));

        for (int i = 0; i < numPointLights; i++) {
            dx::XMVECTOR finalPos = dx::XMVector4Transform(pointLights[i].pointLight.posWorldSpace, pointLights[i].finalWorldTransf);
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_POSITIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_NEGATIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_POSITIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_NEGATIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_POSITIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_NEGATIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

            for (int face = 0; face < 6; face++) {
                rhi.context->ClearDepthStencilView(shadowCubeMapDSV[i * 6 + face], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            }

            for (SubmeshInfo const& sbPair : viewDesc.submeshesInfo) {
                rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
                rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

                ObjCB tmpOCB{ dx::XMMatrixTranspose(sbPair.finalWorldTransf) };
                rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

                RHIState state = RHI_DEFAULT_STATE();
                RHI_RS_SET_CULL_BACK(state);
                RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

                rhi.SetState(state);

                tmpPCB.projMatrix = cbOmniDirShad.pointLightProjMat;
                rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

                for (int face = 0; face < 6; face++) {
                    tmpVCB.viewMatrix = cbOmniDirShad.pointLightViewMat[i * 6 + face];
                    rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);

                    rhi.context->OMSetRenderTargets(0, nullptr, shadowCubeMapDSV[i * 6 + face]);
                    rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
                }

            }
        }
        annot->EndEvent();

    }
    annot->EndEvent();
}


void RendererBackend::RenderView(const ViewDesc& viewDesc)
{
	std::uint32_t strides[] = { sizeof(Vertex) };
	std::uint32_t offsets[] = { 0 };

    rb.annot->BeginEvent(L"Skybox Pass");
    {
        //Using cam rotation matrix as view, to ignore cam translation, making skybox always centered
        ObjCB tmpOCB{ dx::XMMatrixIdentity() };
        ViewCB tmpVCB{ dx::XMMatrixTranspose(viewDesc.cam.getRotationMatrix()) };
        ProjCB tmpPCB{ viewDesc.cam.getProjMatrixTransposed() };

        rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::SKYBOX), 0, 0);
        rhi.context->IASetInputLayout(nullptr);
        rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
        rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
        rhi.context->VSSetConstantBuffers(2, 1, &projCB);

        rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);
        rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
        rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

        rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::SKYBOX), 0, 0);

        rhi.context->PSSetShaderResources(0, 1, &cubemapView);

        RHI_OM_DS_SET_DEPTH_COMP_LESS_EQ(rhiState);
        RHI_RS_SET_CULL_BACK(rhiState);
        rhi.SetState(rhiState);
        rhi.context->Draw(36, 0);
    }
    rb.annot->EndEvent();

	
    std::vector<LightInfo> dirLights;
    std::vector<LightInfo> spotLights;
    std::vector<LightInfo> pointLights;
    //TODO: remove this code here and in RenderShadowMaps; instead order the array of lights in buckets of same type 
    for (const LightInfo& lightInfo : viewDesc.lightsInfo) {
        switch (lightInfo.tag)
        {
            case LightType::DIRECTIONAL:
            {
                dirLights.push_back(lightInfo);
                break;
            }

            case LightType::SPOT:
            {
                spotLights.push_back(lightInfo);
                break;
            }

            case LightType::POINT:
            {
                pointLights.push_back(lightInfo);
                break;
            }
        }
    }

    int numSpotLights = spotLights.size();
    assert(numSpotLights == viewDesc.numSpotLights);

    int numPointLights = pointLights.size();
    assert(numPointLights == viewDesc.numPointLights);

    int numDirLigths = dirLights.size();
    assert(numDirLigths == viewDesc.numDirLights);

    rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
    rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
    rhi.context->VSSetConstantBuffers(2, 1, &projCB);

    rhi.context->IASetInputLayout(shaders.vertexLayout);

    //Rendering shadow maps-------------------------------------------------
    DirShadowCB dirShadowCB{};
    OmniDirShadowCB cbOmniDirShad{};

    RenderShadowMaps(viewDesc, dirShadowCB, cbOmniDirShad);
    //----------------------------------------------------------------------


    //Actual rendering-------------------------------------------------------
    rhi.context->RSSetViewports(1, &sceneViewport);

    ViewCB tmpVCB{ viewDesc.cam.getViewMatrixTransposed() };
    ProjCB tmpPCB{ viewDesc.cam.getProjMatrixTransposed() };
    LightCB tmpLCB;

    for (int i = 0; i < numDirLigths; i++)
    {
        DirectionalLight& dirLight = dirLights.at(i).dirLight;
        dx::XMVECTOR transfDirLight = dx::XMVector4Transform(dirLight.direction, dirLights.at(i).finalWorldTransf);
        tmpLCB.dLight.direction = dx::XMVector4Transform(transfDirLight, viewDesc.cam.getViewMatrix());
        tmpLCB.dLight.shadowMapNearPlane = dirLight.shadowMapNearPlane;
        tmpLCB.dLight.shadowMapFarPlane = dirLight.shadowMapFarPlane;
    }

    for (int i = 0; i < numPointLights; i++) {
        dx::XMVECTOR finalPos = dx::XMVector4Transform(pointLights[i].pointLight.posWorldSpace, pointLights[i].finalWorldTransf);
        tmpLCB.posViewSpace[i] = dx::XMVector4Transform(finalPos, viewDesc.cam.getViewMatrix());
        tmpLCB.pointLights[i].constant = pointLights[i].pointLight.constant;
        tmpLCB.pointLights[i].linear = pointLights[i].pointLight.linear;
        tmpLCB.pointLights[i].quadratic = pointLights[i].pointLight.quadratic;
        tmpLCB.pointLights[i].shadowMapNearPlane = pointLights[i].pointLight.shadowMapNearPlane;
        tmpLCB.pointLights[i].shadowMapFarPlane = pointLights[i].pointLight.shadowMapFarPlane;
    }
    tmpLCB.numPLights = numPointLights;

    for (int i = 0; i < viewDesc.numSpotLights; i++) {
        dx::XMVECTOR finalPos = dx::XMVector4Transform(spotLights[i].spotLight.posWorldSpace, spotLights[i].finalWorldTransf);
        dx::XMVECTOR finalDir = dx::XMVector4Transform(spotLights[i].spotLight.direction, spotLights[i].finalWorldTransf);

        tmpLCB.spotLights[i].cosCutoffAngle = spotLights[i].spotLight.cosCutoffAngle;
        tmpLCB.spotLights[i].posWorldSpace = finalPos;
        tmpLCB.spotLights[i].direction = finalDir;
        tmpLCB.spotLights[i].shadowMapNearPlane = spotLights[i].spotLight.shadowMapNearPlane;
        tmpLCB.spotLights[i].shadowMapFarPlane = spotLights[i].spotLight.shadowMapFarPlane;

        tmpLCB.posSpotViewSpace[i] = dx::XMVector4Transform(finalPos, viewDesc.cam.getViewMatrix());
        tmpLCB.dirSpotViewSpace[i] = dx::XMVector4Transform(finalDir, viewDesc.cam.getViewMatrix());
    }
    tmpLCB.numSpotLights = numSpotLights;

    rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
    rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);
    rhi.context->UpdateSubresource(lightsCB, 0, nullptr, &tmpLCB, 0, 0);
    rhi.context->UpdateSubresource(dirShadCB, 0, nullptr, &dirShadowCB, 0, 0);
    rhi.context->UpdateSubresource(omniDirShadCB, 0, nullptr, &cbOmniDirShad, 0, 0);

    annot->BeginEvent(L"G-Buffer Pass");
    {
        rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::STANDARD), nullptr, 0);
        rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
        rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
        rhi.context->VSSetConstantBuffers(2, 1, &projCB);
        rhi.context->VSSetConstantBuffers(3, 1, &lightsCB);
        rhi.context->VSSetConstantBuffers(4, 1, &dirShadCB);
        rhi.context->VSSetConstantBuffers(5, 1, &omniDirShadCB);

        FLOAT clearCol[4] = { 0.0f,0.0f,0.0f,1.0f };
        for (int i = 0; i < GBuffer::SIZE; i++) {
            rhi.context->ClearRenderTargetView(GBufferRTV[i], clearCol);
        }

        ID3D11RenderTargetView* rt2[4] = { GBufferRTV[0], GBufferRTV[1], GBufferRTV[2], skinRT[4] };
        rhi.context->OMSetRenderTargets(4, &rt2[0]/*&GBufferRTV[0]*/, depthDSV);

        for (SubmeshInfo const& sbPair : viewDesc.submeshesInfo) {
            rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
            rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

            Material& mat = resourceCache.materialCache.at(sbPair.matCacheHnd.index);
            rhi.context->PSSetShader(mat.model.shader, 0, 0);
            rhi.context->PSSetShaderResources(0, 1, &mat.albedoMap.resourceView);
            rhi.context->PSSetShaderResources(1, 1, &mat.normalMap.resourceView);
            rhi.context->PSSetShaderResources(2, 1, &mat.metalnessMap.resourceView);
            rhi.context->PSSetShaderResources(3, 1, &mat.smoothnessMap.resourceView);
            rhi.context->PSSetShaderResources(4, 1, &shadowMapSRV);
            rhi.context->PSSetShaderResources(5, 1, &shadowCubeMapSRV);
            rhi.context->PSSetShaderResources(6, 1, &spotShadowMapSRV);
            rhi.context->PSSetShaderResources(7, 1, &thicknessMapSRV.resourceView);

            rhi.context->UpdateSubresource(matPrmsCB, 0, nullptr, &mat.matPrms, 0, 0);
            rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
            rhi.context->PSSetConstantBuffers(1, 1, &matPrmsCB);
            rhi.context->PSSetConstantBuffers(2, 1, &omniDirShadCB);

            ObjCB tmpOCB{ dx::XMMatrixTranspose(sbPair.finalWorldTransf) };
            rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

            RHIState state = RHI_DEFAULT_STATE();
            RHI_RS_SET_CULL_BACK(state);
            RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

            rhi.SetState(state);

            rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
        }
    }
    annot->EndEvent();


    annot->BeginEvent(L"Lighting Pass");
    {
        ID3D11RenderTargetView* rts[4] = { skinRT[0], skinRT[1], skinRT[2], ambientRTV };
        rhi.context->OMSetRenderTargets(4, &rts[0], nullptr);

        rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::FULL_QUAD), nullptr, 0);
        rhi.context->IASetInputLayout(nullptr);
        rhi.context->IASetVertexBuffers(0, 0, NULL, strides, offsets);
        rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::LIGHTING), nullptr, 0);
        struct Im { dx::XMMATRIX invProj; dx::XMMATRIX invView; };
        Im im;
        im.invProj = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, viewDesc.cam.getProjMatrix()));
        im.invView = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, viewDesc.cam.getViewMatrix()));
        rhi.context->UpdateSubresource(invMatCB, 0, nullptr, &im, 0, 0);
        rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
        rhi.context->PSSetConstantBuffers(5, 1, &invMatCB);
        rhi.context->PSSetShaderResources(0, 3, &GBufferSRV[0]);
        rhi.context->PSSetShaderResources(3, 1, &depthSRV);
        rhi.context->PSSetShaderResources(4, 1, &shadowMapSRV);
        rhi.context->PSSetShaderResources(5, 1, &shadowCubeMapSRV);
        rhi.context->PSSetShaderResources(6, 1, &spotShadowMapSRV);
        rhi.context->PSSetShaderResources(7, 1, &skinSRV[4]);
        rhi.context->Draw(4, 0);
    }
    annot->EndEvent();

    ID3D11ShaderResourceView* nullSRV[8] = { nullptr };

    annot->BeginEvent(L"ShadowMap Pass");
    {
        //rhi.context->PSSetShaderResources(GBuffer::ALBEDO, 1, &nullSRV[0]);
        rhi.context->PSSetShaderResources(GBuffer::ROUGH_MET, 1, &nullSRV[0]);
        ID3D11RenderTargetView* tmpRT[2] = { skinRT[3], GBufferRTV[GBuffer::ROUGH_MET]};
        rhi.context->OMSetRenderTargets(2, &tmpRT[0], nullptr);
        rhi.context->PSSetShaderResources(0, 1, &skinSRV[0]);
        rhi.context->PSSetShaderResources(2, 1, &skinSRV[1]);
        rhi.context->PSSetShaderResources(7, 1, &ambientSRV);
        rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::SHADOW_MAP), nullptr, 0);
        rhi.context->Draw(4, 0);
    }
    annot->EndEvent();

    annot->BeginEvent(L"SSSSS Pass");
    {

        if (viewDesc.submeshesInfo.size() > 0)
        {
            Material& mat = resourceCache.materialCache.at(viewDesc.submeshesInfo.at(0).matCacheHnd.index);

            //for (int i = 0; i < nSamples; i++) {
            //    mat.matPrms.kernel[i] = kernel[i];
            //}
            mat.matPrms.dir = dx::XMFLOAT2(0.0f, 1.0f);
            rhi.context->UpdateSubresource(matPrmsCB, 0, nullptr, &mat.matPrms, 0, 0);

            rhi.context->PSSetShaderResources(2, 1, &nullSRV[0]);
            rhi.context->OMSetRenderTargets(1, /*&skinRT[1]*//*rhi.backBufferRTV.GetAddressOf()*/&finalRenderTargetView, nullptr);
            rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::SSSSS), nullptr, 0);
            //NOTA: skinSRV[3] is diffuse radiance
            annot->BeginEvent(L"mips");
            {
                rhi.context->GenerateMips(skinSRV[3]);
            }
            annot->EndEvent();

            rhi.context->PSSetShaderResources(0, 1, &skinSRV[3]);
            rhi.context->PSSetShaderResources(1, 1, &GBufferSRV[GBuffer::ROUGH_MET]);
            rhi.context->PSSetShaderResources(2, 1, &skinSRV[2]);
            rhi.context->PSSetShaderResources(3, 1, &depthSRV);
            rhi.context->PSSetShaderResources(4, 1, &GBufferSRV[GBuffer::ALBEDO]);
            rhi.context->Draw(4, 0);

            //mat.matPrms.dir = dx::XMFLOAT2(1.0f, 0.0f);
            //rhi.context->UpdateSubresource(matPrmsCB, 0, nullptr, &mat.matPrms, 0, 0);
            //rhi.context->OMSetRenderTargets(1, rhi.renderTargetView.GetAddressOf(), nullptr);
            //rhi.context->PSSetShaderResources(0, 1, &skinSRV[1]);
            //rhi.context->Draw(4, 0); 
    }
        
    }
    annot->EndEvent();

    //rhi.context->OMSetRenderTargets(1, rhi.renderTargetView.GetAddressOf(), nullptr);
    //rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::PRESENT), nullptr, 0);
    //rhi.context->PSSetShaderResources(0, 1, &GBufferSRV[GBuffer::ALBEDO]);
    //rhi.context->Draw(4, 0);


    rhi.context->PSSetShaderResources(0, 8, &nullSRV[0]);

    rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
