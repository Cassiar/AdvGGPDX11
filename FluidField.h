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

	void Simulate();
private:
	int fluidSimGridRes = 8;
	float invFluidSimGridRes = 1.0f / fluidSimGridRes;
	//int groupSize = 8;//8*8*8 =512 the grid res
	
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> velocityMaps[2];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> velocityDivergenceMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pressureMap[2];

	Transform transform;
	std::shared_ptr<SimpleComputeShader> advectionShader;
	std::shared_ptr<SimpleComputeShader> velocityDivergenceShader;
	std::shared_ptr<SimpleComputeShader> PressureSolverShader;
	std::shared_ptr<SimpleComputeShader> PressureProjectionShader;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
};

