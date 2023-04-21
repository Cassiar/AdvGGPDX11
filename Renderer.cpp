#include "Renderer.h"

//currently just setups needed variables.
//in the future it could setup shadow mapping or similar things
Renderer::Renderer(
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
	unsigned int windowWidth, unsigned int windowHeight) 
{
	this->device = device;
	this->context = context;
	this->swapChain = swapChain;
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;
	this->windowWidth = winowWidth;
	this->windowHeight = windowHeight;
}

void Renderer::PreResize() 
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();
}

void Renderer::PostPresize(
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
	unsigned int windowWidht, unsigned int windowHeight) 
{
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;
	this->windowWidth = winowWidth;
	this->windowHeight = windowHeight;
}