#pragma once

#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

#include "Transform.h"
#include "SimpleShader.h"

class FluidField
{
public:
	FluidField(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	~FluidField();

	Transform* GetTransform();

	void Simulate(float deltaTime);
private:
	void SwapBuffers();
	void CreateSRVandUAVTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav);

	int fluidSimGridRes = 64;
	float invFluidSimGridRes = 1.0f / fluidSimGridRes;
	//int groupSize = 8;//8*8*8 =512 the grid res
	
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> velocityMapSRVs[2];
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> velocityMapUAVs[2];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> velocityDivergenceMapSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pressureMapSRVs[2];
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> pressureMapUAVs[2];

	Transform transform;
	std::shared_ptr<SimpleComputeShader> advectionShader;
	std::shared_ptr<SimpleComputeShader> velocityDivergenceShader;
	std::shared_ptr<SimpleComputeShader> PressureSolverShader;
	std::shared_ptr<SimpleComputeShader> PressureProjectionShader;


	Microsoft::WRL::ComPtr<ID3D11SamplerState> bilinearSamplerOptions;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	DirectX::XMFLOAT4* randomPixels;// = new DirectX::XMFLOAT4[fluidSimGridRes * fluidSimGridRes * fluidSimGridRes];
};

