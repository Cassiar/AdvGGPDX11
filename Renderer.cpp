#include "Renderer.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

//currently just sets up needed variables.
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
	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;
}

Renderer::~Renderer()
{
}

void Renderer::PreResize() 
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();
}

void Renderer::FrameStart()
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	// Clear the back buffer (erases what's on the screen)
	const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
	context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

	//also clear any rtvs used for post processing

	// Clear the depth buffer (resets per-pixel occlusion information)
	context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Renderer::FrameEnd(bool vsync)
{
	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Draw the UI after everything else
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		swapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}

void Renderer::RenderScene(
	std::vector<std::shared_ptr<GameEntity>> entities,
	std::vector<Light> lights,
	std::shared_ptr<Sky> sky,
	std::shared_ptr<Camera> camera,
	int lightCount)
{
	// Draw all of the entities
	for (auto& ge : entities)
	{
		// Set the "per frame" data
		// Note that this should literally be set once PER FRAME, before
		// the draw loop, but we're currently setting it per entity since 
		// we are just using whichever shader the current entity has.  
		// Inefficient!!!
		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();
		ps->SetData("lights", (void*)(&lights[0]), sizeof(Light) * lightCount);
		ps->SetInt("lightCount", lightCount);
		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());

		//IBL vars and texs
		ps->SetInt("SpecIBLTotalMipLevels", sky->GetMipLevels());
		ps->SetShaderResourceView("BrdfLookUpMap", sky->GetBRDFLookUpMap());
		ps->SetShaderResourceView("IrradianceIBLMap", sky->GetIrradianceIBLMap());
		ps->SetShaderResourceView("SpecularIBLMap", sky->GetConvolvedSpecularIBLMap());

		ps->CopyBufferData("perFrame");

		// Draw the entity
		ge->Draw(context, camera);
	}

	// Draw the light sources?
	//if (showPointLights)
	//	DrawPointLights();

	// Draw the sky
	sky->Draw(camera);
}

void Renderer::PostResize(
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
	unsigned int windowWidht, unsigned int windowHeight) 
{
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;
	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;
}