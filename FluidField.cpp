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

	const int textureSize = 8;
	const int totalPixels = textureSize * textureSize * textureSize;
	XMFLOAT4 randomPixels[totalPixels] = {};
	for (int i = 0; i < totalPixels; i++) {
		XMVECTOR randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), RandomRange(-1, 1), 0);
		XMStoreFloat4(&randomPixels[i], XMVector3Normalize(randomVec));
	}

	D3D11_TEXTURE2D_DESC td = {};
	td.ArraySize = 1;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.MipLevels = 1;
	td.Height = textureSize;
	td.Width = textureSize;
	td.SampleDesc.Count = 1;

	//init data for the texture
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = randomPixels;
	data.SysMemPitch = sizeof(XMFLOAT4) * 4;

	//create the texture and fill with data
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	device->CreateTexture2D(&td, &data, texture.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = td.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(texture.Get(), &srvDesc, velocityMaps[0].GetAddressOf());
}

FluidField::~FluidField()
{
}

Transform* FluidField::GetTransform()
{
	return &transform;
}

void FluidField::Simulate()
{
}
