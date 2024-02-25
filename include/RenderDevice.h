#ifndef RENDER_DEVICE_H_
#define RENDER_DEVICE_H_

#include <iostream>
#include <cstdint>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>

namespace wrl = Microsoft::WRL;

#define RETURN_IF_FAILED(hResult) { if(FAILED(hResult)) return hResult; }
#define RETURN_IF_FAILED_ZRY(zResult) { if(FAILED(zResult.value)) return zResult; }

//TODO: correct types for all these structs
struct ZRYResult {
	HRESULT value;
};

struct ZRYFormat {
	DXGI_FORMAT value;
};

struct ZRYBindFlags {
	UINT value;
};

struct RenderTextureHandle {
	std::uint64_t index;
};

struct RenderSRVHandle {
	std::uint64_t index;
};

struct RenderRTVHandle {
	std::uint64_t index;
};

struct RenderDSVHandle {
	std::uint64_t index;
};


template <class T>
class RenderDevice {

public:
	ZRYResult createTex2D() {
		return impl().createTex2D();
	}

private:
	T& impl() {
		return *static_cast<T*>(this);
	}

};


class DX11RenderDevice : RenderDevice<DX11RenderDevice> {

public:
	DX11RenderDevice();
	~DX11RenderDevice();

	ZRYResult createTex2D(RenderTextureHandle* texHandle, ZRYBindFlags bindFlags, ZRYFormat format, float width, float height, int numTex = 1, RenderSRVHandle* srvHandle = nullptr, RenderRTVHandle* rtvHandle = nullptr, bool generateMips = false, int mipLevels = 0, int arraySize = 1, int sampleCount = 1, int sampleQuality = 0);
	ZRYResult createSRVTex2D(RenderSRVHandle* srvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int numSRVs = 1, int mipLevels = 1, int mostDetailedMip = 0);
	ZRYResult createRTVTex2D(RenderRTVHandle* rtvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int numRTVs = 1, int mipSlice = 0);
	ZRYResult createDSVTex2D(RenderDSVHandle* dsvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int numDSVs = 1, int mipSlice = 0);

	ZRYResult createTexCubemap(RenderTextureHandle* texHandle, ZRYBindFlags bindFlags, ZRYFormat format, float width, float height, int numTex = 1, RenderSRVHandle* srvHandle = nullptr, RenderRTVHandle* rtvHandle = nullptr, bool generateMips = false, int mipLevels = 0, int arraySize = 1, int sampleCount = 1, int sampleQuality = 0);
	ZRYResult createSRVTex2DArray(RenderSRVHandle* srvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int numSRVs = 1, int arraySize = 1, int firstArraySlice = 0, int mipLevels = 1, int mostDetailedMip = 0);
	ZRYResult createDSVTex2DArray(RenderDSVHandle* dsvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int arraySize, int numDSVs = 1, int mipSlice = 0, int firstArraySlice = 0);

	//TODO: remove this method when rest of abstraction is implemented
	ID3D11Texture2D* getTex2DPointer(const RenderTextureHandle rtHnd) const;
	ID3D11RenderTargetView* getRTVPointer(const RenderRTVHandle rtvHnd) const;
	ID3D11ShaderResourceView* getSRVPointer(const RenderSRVHandle srvHnd) const;
	ID3D11DepthStencilView* getDSVPointer(const RenderDSVHandle dsvHnd) const;

	//TODO: move to private when you add all the functionality in this abstraction layer
	wrl::ComPtr<ID3D11Device> device;

private:
	
	std::vector<ID3D11Texture2D*> tex2DResources;
	std::vector<ID3D11RenderTargetView*> rtvResources;
	std::vector<ID3D11ShaderResourceView*> srvResources;
	std::vector<ID3D11DepthStencilView*> dsvResources;

	int tex2dCount, rtvCount, srvCount, dsvCount;

};



#endif