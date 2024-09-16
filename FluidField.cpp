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
	pressureSolverShader = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), FixPath(L"PressureSolverCS.cso").c_str());
	pressureProjectionShader = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), FixPath(L"PressureProjectionCS.cso").c_str());

	//initialize random values for velocity and density map

	const int textureSize = 64;
	const int totalPixels = textureSize * textureSize *textureSize;
	randomPixels = new XMFLOAT4[totalPixels];
	randomPixelsPressure = new XMFLOAT4[totalPixels];
	randomPixelsDensity = new XMFLOAT4[totalPixels];
	for (int i = 0; i < totalPixels; i++) {
		XMVECTOR randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), RandomRange(-1, 1), 0);
		XMStoreFloat4(&randomPixels[i], XMVector3Normalize(randomVec));

		randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), RandomRange(-1, 1), 0);
		XMStoreFloat4(&randomPixelsPressure[i], XMVector3Normalize(randomVec));

		randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), RandomRange(-1, 1), 0);
		XMStoreFloat4(&randomPixelsDensity[i], XMVector3Normalize(randomVec));
	}

	D3D11_TEXTURE3D_DESC desc = {};
	desc.Width = fluidSimGridRes;
	desc.Height = fluidSimGridRes;
	desc.Depth = fluidSimGridRes;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;

	//init data for the texture
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = randomPixels;
	data.SysMemPitch = sizeof(XMFLOAT4) * fluidSimGridRes;
	data.SysMemSlicePitch = sizeof(XMFLOAT4) * fluidSimGridRes * fluidSimGridRes;

	D3D11_SUBRESOURCE_DATA pressureData = {};
	pressureData.pSysMem = randomPixelsPressure;
	pressureData.SysMemPitch = sizeof(XMFLOAT4) * fluidSimGridRes;
	pressureData.SysMemSlicePitch = sizeof(XMFLOAT4) * fluidSimGridRes * fluidSimGridRes;
	
	D3D11_SUBRESOURCE_DATA densityData = {};
	densityData.pSysMem = randomPixelsDensity;
	densityData.SysMemPitch = sizeof(XMFLOAT4) * fluidSimGridRes;
	densityData.SysMemSlicePitch = sizeof(XMFLOAT4) * fluidSimGridRes * fluidSimGridRes;

	//create the texture and fill with data
	Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityTex1;
	device->CreateTexture3D(&desc, &data, velocityTex1.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityTex2;
	device->CreateTexture3D(&desc, 0, velocityTex2.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11Texture3D> densityTex1;
	device->CreateTexture3D(&desc, &densityData, densityTex1.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture3D> densityTex2;
	device->CreateTexture3D(&desc, 0, densityTex2.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityDivergenceTex;
	device->CreateTexture3D(&desc, 0, velocityDivergenceTex.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11Texture3D> pressureTex1;
	device->CreateTexture3D(&desc, &pressureData, pressureTex1.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture3D> pressureTex2;
	device->CreateTexture3D(&desc, 0, pressureTex2.GetAddressOf());


	velocityMap[0] = CreateSRVandUAVTexture(randomPixels);
	velocityMap[1] = CreateSRVandUAVTexture(0);

	densityMap[0] = CreateSRVandUAVTexture(randomPixelsDensity);
	densityMap[1] = CreateSRVandUAVTexture(0);

	velocityDivergenceMap = CreateSRVandUAVTexture(0);

	pressureMap[0] = CreateSRVandUAVTexture(randomPixelsPressure);
	pressureMap[1] = CreateSRVandUAVTexture(0);

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	device->CreateSamplerState(&sampDesc, linearClampSamplerOptions.GetAddressOf());

	D3D11_SAMPLER_DESC bilinearSampDesc = {};
	bilinearSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	bilinearSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	bilinearSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	bilinearSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	bilinearSampDesc.MaxAnisotropy = 16;
	bilinearSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&bilinearSampDesc, bilinearSamplerOptions.GetAddressOf());

	D3D11_SAMPLER_DESC pointSampDesc = {};
	pointSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;;
	pointSampDesc.MaxAnisotropy = 16;
	pointSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&pointSampDesc, pointSamplerOptions.GetAddressOf());

	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	device->CreateDepthStencilState(&depthDesc, depthState.GetAddressOf());

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.CullMode = D3D11_CULL_FRONT;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.DepthClipEnable = true;
	device->CreateRasterizerState(&rasterDesc, rasterState.GetAddressOf());

	cube = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device);
}

FluidField::~FluidField()
{
	if (randomPixels) free(randomPixels);
	if (randomPixelsPressure) free(randomPixelsPressure);
	if (randomPixelsDensity) free(randomPixelsDensity);
}

Transform* FluidField::GetTransform()
{
	return &transform;
}

void FluidField::Simulate(float deltaTime)
{
	//velocity advection
	{
		advectionShader->SetShader();
		advectionShader->SetFloat("deltaTime", deltaTime);
		advectionShader->SetFloat("invFluidSimGridRes", invFluidSimGridRes);

		advectionShader->CopyBufferData("ExternalData");

		advectionShader->SetShaderResourceView("InputMap", velocityMap[0].srv.Get());
		advectionShader->SetShaderResourceView("VelocityMap", velocityMap[0].srv.Get());
		advectionShader->SetUnorderedAccessView("UavOutputMap", velocityMap[1].uav.Get());

		advectionShader->SetSamplerState("BilinearSampler", bilinearSamplerOptions.Get());

		advectionShader->DispatchByThreads(fluidSimGridRes, fluidSimGridRes, fluidSimGridRes);

		//unbind textures	
		advectionShader->SetShaderResourceView("InputMap", 0);
		advectionShader->SetShaderResourceView("VelocityMap", 0);
		advectionShader->SetUnorderedAccessView("UavOutputMap", 0);
	}

	//density advection
	{
		advectionShader->SetShader();
		advectionShader->SetFloat("deltaTime", deltaTime);
		advectionShader->SetFloat("invFluidSimGridRes", invFluidSimGridRes);

		advectionShader->CopyBufferData("ExternalData");

		advectionShader->SetShaderResourceView("InputMap", densityMap[0].srv.Get());
		advectionShader->SetShaderResourceView("VelocityMap", velocityMap[0].srv.Get());
		advectionShader->SetUnorderedAccessView("UavOutputMap", densityMap[1].uav.Get());

		advectionShader->SetSamplerState("BilinearSampler", bilinearSamplerOptions.Get());

		advectionShader->DispatchByThreads(fluidSimGridRes, fluidSimGridRes, fluidSimGridRes);

		//unbind textures	
		advectionShader->SetShaderResourceView("InputMap", 0);
		advectionShader->SetShaderResourceView("VelocityMap", 0);
		advectionShader->SetUnorderedAccessView("UavOutputMap", 0);
	}

	//velocity divergence
	{
		velocityDivergenceShader->SetShader();
		velocityDivergenceShader->SetFloat("deltaTime", deltaTime);
		velocityDivergenceShader->SetFloat("invFluidSimGridRes", invFluidSimGridRes);
		velocityDivergenceShader->SetFloat("gridRes", fluidSimGridRes);

		velocityDivergenceShader->CopyBufferData("ExternalData");

		velocityDivergenceShader->SetShaderResourceView("VelocityMap", velocityMap[1].srv.Get());
		velocityDivergenceShader->SetUnorderedAccessView("UavOutputMap", velocityDivergenceMap.uav.Get());

		velocityDivergenceShader->SetSamplerState("PointSampler", pointSamplerOptions.Get());

		velocityDivergenceShader->DispatchByThreads(fluidSimGridRes, fluidSimGridRes, fluidSimGridRes);

		//unbind textures	
		velocityDivergenceShader->SetShaderResourceView("VelocityMap", 0);
		velocityDivergenceShader->SetUnorderedAccessView("UavOutputMap", 0);
	}

	//pressure solver, run like 20 times
	for (int i = 0; i < 20; i++) {
		pressureSolverShader->SetShader();
		
		pressureSolverShader->SetFloat("deltaTime", deltaTime);
		pressureSolverShader->SetFloat("invFluidSimGridRes", invFluidSimGridRes);
		pressureSolverShader->SetFloat("gridRes", fluidSimGridRes);

		pressureSolverShader->CopyBufferData("ExternalData");

		pressureSolverShader->SetShaderResourceView("VelocityDivergenceMap", velocityDivergenceMap.srv.Get());
		pressureSolverShader->SetShaderResourceView("PressureMap", pressureMap[0].srv.Get());
		pressureSolverShader->SetUnorderedAccessView("UavOutputMap", pressureMap[1].uav.Get());

		pressureSolverShader->SetSamplerState("PointSampler", pointSamplerOptions.Get());
		pressureSolverShader->SetSamplerState("BilinearSampler", bilinearSamplerOptions.Get());

		//dispatch and unbind
		pressureSolverShader->DispatchByThreads(fluidSimGridRes, fluidSimGridRes, fluidSimGridRes);

		pressureSolverShader->SetShaderResourceView("VelocityDivergenceMap", 0);
		pressureSolverShader->SetShaderResourceView("PressureMap", 0);
		pressureSolverShader->SetUnorderedAccessView("UavOutputMap", 0);

		//swap pressure buffers for next solver pass
		SwapBuffers(pressureMap);
	}

	//pressure projection
	{
		pressureProjectionShader->SetShader();

		pressureProjectionShader->SetFloat("deltaTime", deltaTime);
		pressureProjectionShader->SetFloat("invFluidSimGridRes", invFluidSimGridRes);
		pressureProjectionShader->SetFloat("gridRes", fluidSimGridRes);

		pressureProjectionShader->CopyBufferData("ExternalData");

		pressureProjectionShader->SetShaderResourceView("VelocityMap", velocityMap[1].srv.Get());
		pressureProjectionShader->SetShaderResourceView("PressureMap", pressureMap[0].srv.Get());
		pressureProjectionShader->SetUnorderedAccessView("UavOutputMap", velocityMap[0].uav.Get());

		pressureProjectionShader->SetSamplerState("PointSampler", pointSamplerOptions.Get());

		//dispatch and unbind
		pressureProjectionShader->DispatchByThreads(fluidSimGridRes, fluidSimGridRes, fluidSimGridRes);

		pressureProjectionShader->SetShaderResourceView("VelocityMap", 0);
		pressureProjectionShader->SetShaderResourceView("PressureMap", 0);
		pressureProjectionShader->SetUnorderedAccessView("UavOutputMap", 0);
	}
	//SwapBuffers();
	//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> temp = densityMapSRVs[0];
	//densityMapSRVs[0] = densityMapSRVs[1];
	//densityMapSRVs[1] = temp;
	//
	//Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uavTemp = densityMapUAVs[0];
	//densityMapUAVs[0] = densityMapUAVs[1];
	//densityMapUAVs[1] = uavTemp;
}

void FluidField::RenderFluid(Camera* camera) {
	context->OMSetDepthStencilState(depthState.Get(), 0);
	context->OMSetBlendState(blendState.Get(), 0, 0xFFFFFFFF);
	context->RSSetState(rasterState.Get());

	//get the smallest dimension and use for scaling
	//which for now is the same since we have a perfect cube
	float smallestDimension = fluidSimGridRes;
	XMFLOAT3 scale = { fluidSimGridRes, fluidSimGridRes, fluidSimGridRes };

	//cube location
	XMFLOAT3 translation(0, 0, 0);

	volumePS->SetShader();
	volumeVS->SetShader();

	//world mat for vertex shader
	XMMATRIX worldMat = XMMatrixScaling(scale.x, scale.y, scale.z) *
		XMMatrixTranslation(translation.x, translation.y, translation.z);

	XMFLOAT4X4 world, invWorld;
	XMStoreFloat4x4(&world, worldMat);
	XMStoreFloat4x4(&invWorld, XMMatrixInverse(0, worldMat));
	volumeVS->SetMatrix4x4("world", world);
	volumeVS->SetMatrix4x4("view", camera->GetView());
	volumeVS->SetMatrix4x4("projection", camera->GetProjection());
	volumeVS->CopyAllBufferData();
	//should be linear clamp
	volumeVS->SetSamplerState("SamplerLinearClamp", linearClampSamplerOptions);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv = densityMap[0].srv;
	//this where code to switch which srv is being displayed would go

	volumePS->SetShaderResourceView("volumeTexture", srv);

	volumePS->SetMatrix4x4("invWorld", invWorld);
	volumePS->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
	volumePS->SetFloat3("fluidColor", fluidColor);
	volumePS->SetInt("renderMode", -1);
	volumePS->SetInt("raymarchSamples", raymarchSamples);
	volumePS->CopyAllBufferData();

	//cube mesh to render fluid within
	cube->SetBufferAndDraw(context);


	// Reset render states
	context->OMSetDepthStencilState(0, 0);
	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	context->RSSetState(0);
}

void FluidField::SwapBuffers(VolumeResource vr[2]) {
	//swap the buffers
	VolumeResource temp = vr[0];
	vr[0] = vr[1];
	vr[1] = temp;
}

void FluidField::SwapPressureBuffers() {
	//swap the buffers
	//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> temp = pressureMapSRVs[0];
	//pressureMapSRVs[0] = pressureMapSRVs[1];
	//pressureMapSRVs[1] = temp;
	//
	//Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uavTemp = pressureMapUAVs[0];
	//pressureMapUAVs[0] = pressureMapUAVs[1];
	//pressureMapUAVs[1] = uavTemp;
}

FluidField::VolumeResource FluidField::CreateSRVandUAVTexture(void* initialData) {

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
	if (initialData) {
		data.pSysMem = initialData;
		data.SysMemPitch = sizeof(XMFLOAT4) * fluidSimGridRes;
		data.SysMemSlicePitch = sizeof(XMFLOAT4) * fluidSimGridRes * fluidSimGridRes;
	}

	//create the texture and fill with data
	Microsoft::WRL::ComPtr<ID3D11Texture3D> texture;
	device->CreateTexture3D(&desc, initialData ? &data : 0, texture.GetAddressOf());

	VolumeResource vr;
	device->CreateShaderResourceView(texture.Get(), 0, vr.srv.GetAddressOf());
	device->CreateUnorderedAccessView(texture.Get(), 0, vr.uav.GetAddressOf());
	return vr;
}
