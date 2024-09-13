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
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* GetDensityMap() {
		return &densityMap[0].srv;
	};
private:
	struct VolumeResource {
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
	};

	/// <summary>
	/// Swap all buffers that require previous and current frame info
	/// </summary>
	void SwapBuffers(VolumeResource vr[2]);

	/// <summary>
	/// Swap just the pressure buffers for the pressure solver
	/// </summary>
	void SwapPressureBuffers();

	/// <summary>
	/// Helper fucntion to create paired SRVs and UAVs for fluid sim
	/// </summary>
	VolumeResource CreateSRVandUAVTexture(void* initialData);// Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav);

	int fluidSimGridRes = 64;
	float invFluidSimGridRes = 1.0f / fluidSimGridRes;
	//int groupSize = 8;//8*8*8 =512 the grid res
	
	VolumeResource velocityMap[2];

	VolumeResource densityMap[2];

	VolumeResource velocityDivergenceMap;

	VolumeResource pressureMap[2];

	Transform transform;
	std::shared_ptr<SimpleComputeShader> advectionShader;
	std::shared_ptr<SimpleComputeShader> velocityDivergenceShader;
	std::shared_ptr<SimpleComputeShader> pressureSolverShader;
	std::shared_ptr<SimpleComputeShader> pressureProjectionShader;


	Microsoft::WRL::ComPtr<ID3D11SamplerState> bilinearSamplerOptions;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSamplerOptions;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	DirectX::XMFLOAT4* randomPixels;// = new DirectX::XMFLOAT4[fluidSimGridRes * fluidSimGridRes * fluidSimGridRes];
	DirectX::XMFLOAT4* randomPixelsPressure;
	DirectX::XMFLOAT4* randomPixelsDensity;
};

