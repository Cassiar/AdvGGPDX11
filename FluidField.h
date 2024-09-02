#pragma once

#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

class FluidField
{
public:
private:
	int fluidSimGridRes = 512;
	float invFluidSimGridRes = 1.0f / fluidSimGridRes;
	
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> velocityMaps[2];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> velocityDivergenceMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pressureMap[2];
};

