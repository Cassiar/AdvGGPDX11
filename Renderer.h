#pragma once
//@author: cassiar
// a renderer class to hold all rendering
// info and help easy postprocess work

#include "SimpleShader.h"
#include "GameEntity.h"
#include "Lights.h"
#include "Camera.h"
#include "Sky.h"

#include <Windows.h>
#include <d3d11.h>
#include <string>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>

// We can include the correct library files here
// instead of in Visual Studio settings if we want
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

//enum to allow easy access to rtv names
enum RenderTargetType {
	SCENE_COLORS_NO_AMBIENT,
	SCENE_AMBIENT,
	SCENE_DEPTH,
	SCENE_NORMALS,
	SSAO_RESULTS,
	SSAO_BLUR,

	//this will allways equal count since enums start at 0
	TYPE_COUNT
};

class Renderer
{
public:
	Renderer(
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
		unsigned int windowWidth, unsigned int windowHeight);
	~Renderer();

	//calls reset on rtv and dsv
	void PreResize();

	//update fields with the new values
	void PostResize(
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
		unsigned int windowWidth, unsigned int windowHeight);

	//handle begining of fram
	//e.g., clearing buffers
	void FrameStart();
	void FrameEnd(bool vsync);
	void RenderScene(
		std::vector<std::shared_ptr<GameEntity>> entities,
		std::vector<Light> lights,
		std::shared_ptr<Sky> sky,
		std::shared_ptr<Camera> camera,
		int lightCount);

private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;

	unsigned int windowWidth;
	unsigned int windowHeight;

	//middle rtv & srv to hold info before post processing
	//six in total for ssao
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRTVs[RenderTargetType::TYPE_COUNT];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSRVs[RenderTargetType::TYPE_COUNT];

	int numOffsets = 64;
	DirectX::XMFLOAT4 ssaoOffsets[64];

	void CreateRenderTarget(unsigned int width, unsigned int height,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
		DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
};

