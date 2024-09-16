#pragma once

#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

#include "Transform.h"
#include "SimpleShader.h"
#include "Camera.h"
#include "Mesh.h"

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

	void RenderFluid(Camera* camera);
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

	DirectX::XMFLOAT4* randomPixels;// = new DirectX::XMFLOAT4[fluidSimGridRes * fluidSimGridRes * fluidSimGridRes];
	DirectX::XMFLOAT4* randomPixelsPressure;
	DirectX::XMFLOAT4* randomPixelsDensity;

	DirectX::XMFLOAT3 fluidColor = { 1.0f, 1.0f, 1.0f };
	int raymarchSamples = 128;
	Transform transform;

	std::shared_ptr<Mesh> cube;

	VolumeResource velocityMap[2];

	VolumeResource densityMap[2];

	VolumeResource velocityDivergenceMap;

	VolumeResource pressureMap[2];

	std::shared_ptr<SimpleComputeShader> advectionShader;
	std::shared_ptr<SimpleComputeShader> velocityDivergenceShader;
	std::shared_ptr<SimpleComputeShader> pressureSolverShader;
	std::shared_ptr<SimpleComputeShader> pressureProjectionShader;

	//shaders to render the fluid
	std::shared_ptr<SimplePixelShader> volumePS;
	std::shared_ptr<SimpleVertexShader> volumeVS;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> linearClampSamplerOptions;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> bilinearSamplerOptions;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSamplerOptions;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthState;
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState;

};

