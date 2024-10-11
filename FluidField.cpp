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
	volumeVS = std::make_shared<SimpleVertexShader>(device.Get(), context.Get(), FixPath(L"VolumeVS.cso").c_str());
	volumePS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), FixPath(L"VolumePS.cso").c_str());

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
	//Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityTex1;
	//device->CreateTexture3D(&desc, &data, velocityTex1.GetAddressOf());
	//Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityTex2;
	//device->CreateTexture3D(&desc, 0, velocityTex2.GetAddressOf());

	//Microsoft::WRL::ComPtr<ID3D11Texture3D> densityTex1;
	//device->CreateTexture3D(&desc, &densityData, densityTex1.GetAddressOf());
	//Microsoft::WRL::ComPtr<ID3D11Texture3D> densityTex2;
	//device->CreateTexture3D(&desc, 0, densityTex2.GetAddressOf());

	//Microsoft::WRL::ComPtr<ID3D11Texture3D> velocityDivergenceTex;
	//device->CreateTexture3D(&desc, 0, velocityDivergenceTex.GetAddressOf());

	//Microsoft::WRL::ComPtr<ID3D11Texture3D> pressureTex1;
	//device->CreateTexture3D(&desc, &pressureData, pressureTex1.GetAddressOf());
	//Microsoft::WRL::ComPtr<ID3D11Texture3D> pressureTex2;
	//device->CreateTexture3D(&desc, 0, pressureTex2.GetAddressOf());


	velocityMap[0] = CreateSRVandUAVTexture(DXGI_FORMAT_R32G32B32A32_FLOAT, randomPixels);
	velocityMap[1] = CreateSRVandUAVTexture(DXGI_FORMAT_R32G32B32A32_FLOAT, 0);

	densityMap[0] = CreateSRVandUAVTexture(DXGI_FORMAT_R32G32B32A32_FLOAT, randomPixelsDensity);
	densityMap[1] = CreateSRVandUAVTexture(DXGI_FORMAT_R32G32B32A32_FLOAT, 0);

	velocityDivergenceMap = CreateSRVandUAVTexture(DXGI_FORMAT_R32_FLOAT, 0);

	pressureMap[0] = CreateSRVandUAVTexture(DXGI_FORMAT_R32_FLOAT, randomPixelsPressure);
	pressureMap[1] = CreateSRVandUAVTexture(DXGI_FORMAT_R32_FLOAT, 0);

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	device->CreateSamplerState(&sampDesc, linearClampSamplerOptions.GetAddressOf());

	//D3D11_SAMPLER_DESC bilinearSampDesc = {};
	//bilinearSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	//bilinearSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	//bilinearSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	//bilinearSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	//bilinearSampDesc.MaxAnisotropy = 16;
	//bilinearSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	//device->CreateSamplerState(&bilinearSampDesc, bilinearSamplerOptions.GetAddressOf());

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

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&blendDesc, blendState.GetAddressOf());

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
		velocityDivergenceShader->SetFloat("gridRes", (float)fluidSimGridRes);

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
		pressureSolverShader->SetFloat("gridRes", (float)fluidSimGridRes);

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
		pressureProjectionShader->SetFloat("gridRes", (float)fluidSimGridRes);

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
	
	SwapBuffers(densityMap);
}

void FluidField::RenderFluid(std::shared_ptr<Camera> camera) {
	context->OMSetDepthStencilState(depthState.Get(), 0);
	context->OMSetBlendState(blendState.Get(), 0, 0xFFFFFFFF);
	context->RSSetState(rasterState.Get());

	//get the smallest dimension and use for scaling
	//which for now is the same since we have a perfect cube
	float smallestDimension = (float)fluidSimGridRes;
	//XMFLOAT3 scale = { (float)fluidSimGridRes, (float)fluidSimGridRes, (float)fluidSimGridRes };
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

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

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv = densityMap[1].srv;
	//this where code to switch which srv is being displayed would go

	volumePS->SetShaderResourceView("VolumeTexture", srv);

	volumePS->SetMatrix4x4("invWorld", invWorld);
	volumePS->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
	volumePS->SetFloat3("fluidColor", fluidColor);
	volumePS->SetInt("renderMode", 0);//blend in ps
	volumePS->SetInt("raymarchSamples", raymarchSamples);
	volumePS->CopyAllBufferData();

	//cube mesh to render fluid within
	cube->SetBuffersAndDraw(context);


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

FluidField::VolumeResource FluidField::CreateSRVandUAVTexture(DXGI_FORMAT format, void* initialData) {

	D3D11_TEXTURE3D_DESC desc = {};
	desc.Width = fluidSimGridRes;
	desc.Height = fluidSimGridRes;
	desc.Depth = fluidSimGridRes;
	desc.Format = format;
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
		data.SysMemPitch = DXGIFormatBytes(format) * fluidSimGridRes;
		data.SysMemSlicePitch = DXGIFormatBytes(format) * fluidSimGridRes * fluidSimGridRes;
	}

	//create the texture and fill with data
	Microsoft::WRL::ComPtr<ID3D11Texture3D> texture;
	device->CreateTexture3D(&desc, initialData ? &data : 0, texture.GetAddressOf());

	VolumeResource vr;
	device->CreateShaderResourceView(texture.Get(), 0, vr.srv.GetAddressOf());
	device->CreateUnorderedAccessView(texture.Get(), 0, vr.uav.GetAddressOf());
	return vr;
}

// From DirectXTex library
unsigned int FluidField::DXGIFormatBits(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
		return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_AYUV:
	case DXGI_FORMAT_Y410:
	case DXGI_FORMAT_YUY2:
		return 32;

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
		return 24;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return 16;

	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
	case DXGI_FORMAT_NV11:
		return 12;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8:
		return 8;

	case DXGI_FORMAT_R1_UNORM:
		return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 4;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 8;

	default:
		return 0;
	}
}

unsigned int FluidField::DXGIFormatBytes(DXGI_FORMAT format)
{
	unsigned int bits = DXGIFormatBits(format);
	if (bits == 0)
		return 0;

	return max(1, bits / 8);
}

unsigned int FluidField::DXGIFormatChannels(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return 4;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_B5G6R5_UNORM:
		return 3;


	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_A8P8:
		return 2;

	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_AYUV:
	case DXGI_FORMAT_Y410:
	case DXGI_FORMAT_YUY2:
	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
	case DXGI_FORMAT_NV11:
	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8:
	case DXGI_FORMAT_R1_UNORM:
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 1;

	default:
		return 0;
	}
}
