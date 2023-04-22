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

#pragma once
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
		unsigned int windowWidht, unsigned int windowHeight);

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

	//middle rtv for hold info before post processing
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneColorRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneAmbientColorRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneDepthsRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneNormalsRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ssaoRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> blurredSsaoRTV;

	void MakeRTV(Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv);
};

