#include "FluidField.h"
#include "Helpers.h"

using namespace DirectX;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

FluidField::FluidField(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	this->device = device;
	this->context = context;

	//create simple shader objects
	advectionShader = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), FixPath(L"AdvectionCS.cso").c_str());
	velocityDivergenceShader = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), FixPath(L"VelocityDivergenceCS.cso").c_str());
	PressureSolverShader = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), FixPath(L"PressureSolverCS.cso").c_str());
	PressureProjectionShader = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), FixPath(L"PressureProjectionCS.cso").c_str());

	//initialize random values for velocity and density map

	const int textureSize = 64;
	const int totalPixels = textureSize * textureSize *textureSize;
	randomPixels = new XMFLOAT4[totalPixels];
	for (int i = 0; i < 512; i++) {
		XMVECTOR randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), RandomRange(-1, 1), 0);
		XMStoreFloat4(&randomPixels[i], randomVec);
	}

	D3D11_TEXTURE3D_DESC desc = {};
	desc.Width = fluidSimGridRes;
	desc.Height = fluidSimGridRes;
	desc.Depth = fluidSimGridRes;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;

	//D3D11_TEXTURE3D_DESC td = {};
	////td.ArraySize = 1;
	//td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	//td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//td.MipLevels = 1;
	//td.Height = fluidSimGridRes;
	//td.Width = fluidSimGridRes;
	////td.SampleDesc.Count = 1;
	//td.Depth = fluidSimGridRes;

	//init data for the texture
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = randomPixels;
	data.SysMemPitch = sizeof(XMFLOAT4) * fluidSimGridRes;
	data.SysMemSlicePitch = sizeof(XMFLOAT4) * fluidSimGridRes * fluidSimGridRes;

	//create the texture and fill with data
	Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityTex1;
	device->CreateTexture3D(&desc, &data, velocityTex1.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityTex2;
	device->CreateTexture3D(&desc, 0, velocityTex2.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityDivergenceTex;
	device->CreateTexture3D(&desc, 0, velocityDivergenceTex.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11Texture3D> pressureTex1;
	device->CreateTexture3D(&desc, 0, pressureTex1.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture3D> pressureTex2;
	device->CreateTexture3D(&desc, 0, pressureTex2.GetAddressOf());


	device->CreateShaderResourceView(velocityTex1.Get(), 0, velocityMapSRVs[0].GetAddressOf());
	device->CreateUnorderedAccessView(velocityTex1.Get(), 0, velocityMapUAVs[0].GetAddressOf());
	device->CreateShaderResourceView(velocityTex2.Get(), 0, velocityMapSRVs[1].GetAddressOf());
	device->CreateUnorderedAccessView(velocityTex2.Get(), 0, velocityMapUAVs[1].GetAddressOf());


	device->CreateShaderResourceView(velocityDivergenceTex.Get(), 0, velocityDivergenceMapSRV.GetAddressOf());
	device->CreateUnorderedAccessView(velocityDivergenceTex.Get(), 0, velocityDivergenceMapUAV.GetAddressOf());

	device->CreateShaderResourceView(pressureTex1.Get(), 0, pressureMapSRVs[0].GetAddressOf());
	device->CreateUnorderedAccessView(pressureTex1.Get(), 0, pressureMapUAVs[0].GetAddressOf());
	device->CreateShaderResourceView(pressureTex2.Get(), 0, pressureMapSRVs[1].GetAddressOf());
	device->CreateUnorderedAccessView(pressureTex2.Get(), 0, pressureMapUAVs[1].GetAddressOf());

	//CreateSRVandUAVTexture(device, velocityMapSRVs[1], velocityMapUAVs[1]);

	D3D11_SAMPLER_DESC bilinearSampDesc = {};
	bilinearSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	bilinearSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	bilinearSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	bilinearSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	bilinearSampDesc.MaxAnisotropy = 16;
	bilinearSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&bilinearSampDesc, bilinearSamplerOptions.GetAddressOf());
}

FluidField::~FluidField()
{
	free(randomPixels);
}

Transform* FluidField::GetTransform()
{
	return &transform;
}

void FluidField::Simulate(float deltaTime)
{
	advectionShader->SetShader();
	advectionShader->SetFloat("deltaTime", deltaTime);
	advectionShader->SetFloat("invFluidSimGridRes", invFluidSimGridRes);

	advectionShader->CopyBufferData("ExternalData");

	advectionShader->SetShaderResourceView("InputMap", velocityMapSRVs[0].Get());
	advectionShader->SetShaderResourceView("VelocityMap", velocityMapSRVs[0].Get());
	advectionShader->SetUnorderedAccessView("UavOutputMap", velocityMapUAVs[1].Get());

	advectionShader->SetSamplerState("BilinearSampler", bilinearSamplerOptions.Get());

	advectionShader->DispatchByThreads(8, 8, 8);

	//unbind textures	
	advectionShader->SetShaderResourceView("InputMap", 0);
	advectionShader->SetShaderResourceView("VelocityMap", 0);
	advectionShader->SetUnorderedAccessView("UavOutputMap", 0);

	//velocityDivergenceShader->SetShader();

	SwapBuffers();
}

void FluidField::SwapBuffers() {
	//swap the buffers
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> temp = velocityMapSRVs[0];
	velocityMapSRVs[0] = velocityMapSRVs[1];
	velocityMapSRVs[1] = temp;

	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uavTemp = velocityMapUAVs[0];
	velocityMapUAVs[0] = velocityMapUAVs[1];
	velocityMapUAVs[1] = uavTemp;
}

void FluidField::CreateSRVandUAVTexture(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav) {

	D3D11_TEXTURE3D_DESC desc = {};
	desc.Width = fluidSimGridRes;
	desc.Height = fluidSimGridRes;
	desc.Depth = fluidSimGridRes;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;

	//D3D11_TEXTURE3D_DESC td = {};
	////td.ArraySize = 1;
	//td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	//td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//td.MipLevels = 1;
	//td.Height = fluidSimGridRes;
	//td.Width = fluidSimGridRes;
	////td.SampleDesc.Count = 1;
	//td.Depth = fluidSimGridRes;

	//init data for the texture
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = randomPixels;
	data.SysMemPitch = sizeof(XMFLOAT4) * fluidSimGridRes;
	data.SysMemSlicePitch = sizeof(XMFLOAT4) * fluidSimGridRes * fluidSimGridRes;

	//create the texture and fill with data
	Microsoft::WRL::ComPtr<ID3D11Texture3D> texture;
	device->CreateTexture3D(&desc, &data, texture.GetAddressOf());

	//D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	//srvDesc.Format = td.Format;
	//srvDesc.Texture3D.MipLevels = 1;
	//srvDesc.Texture3D.MostDetailedMip = 0;

	//D3D11_TEXTURE3D_DESC tdUAV = {};
	////tdUAV.ArraySize = 1;
	//tdUAV.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	//tdUAV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//tdUAV.MipLevels = 1;
	//tdUAV.Height = fluidSimGridRes;
	//tdUAV.Width = fluidSimGridRes;
	////tdUAV.SampleDesc.Count = 1;
	//tdUAV.Depth = fluidSimGridRes;
	//
	//// 
	////create the texture and fill with data
	//Microsoft::WRL::ComPtr<ID3D11Texture3D> textureUAV;
	//device->CreateTexture3D(&tdUAV, &data, textureUAV.GetAddressOf());

	//D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	//uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	//uavDesc.Format = tdUAV.Format;
	//uavDesc.Texture3D.FirstWSlice = 0;
	//uavDesc.Texture3D.WSize = -1;

	device->CreateShaderResourceView(texture.Get(), 0, srv.GetAddressOf());
	device->CreateUnorderedAccessView(texture.Get(), 0, uav.GetAddressOf());
}
