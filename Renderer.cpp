#include "Renderer.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

using namespace DirectX;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

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


	//call post resize since it recreates render targets
	this->PostResize(backBufferRTV, depthBufferDSV, windowWidth, windowHeight);

	//create array of offsets ssao will use
	for (int i = 0; i < numOffsets; i++) {
		ssaoOffsets[i] = XMFLOAT4(
			(float)rand() / RAND_MAX * 2 - 1, //gets range -1 to 1
			(float)rand() / RAND_MAX * 2 - 1,
			(float)rand() / RAND_MAX, //range 0 to 1
			0);

		XMVECTOR offset = XMVector3Normalize(XMLoadFloat4(&ssaoOffsets[i]));

		//scale so that more values are closer to min than max
		float scale = (float)i / numOffsets;
		XMVECTOR acceleratedScale = XMVectorLerp(
			XMVectorSet(0.1f, 0.1f, 0.1f, 1),
			XMVectorSet(1, 1, 1, 1),
			scale * scale);
		XMStoreFloat4(&ssaoOffsets[i], offset * acceleratedScale);
	}
}

Renderer::~Renderer()
{
}

//create the MRTs needed for ssao post processing
void Renderer::CreateRenderTarget(
	unsigned int width, unsigned int height,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
	DXGI_FORMAT colorFormat)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;

	D3D11_TEXTURE2D_DESC texDesc = {};
	//set width and height of texture
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; //both rendering in, and sampling
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = colorFormat;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;

	device->CreateTexture2D(&texDesc, 0, tex.GetAddressOf());

	// Make the render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // This points to a Texture2D
	rtvDesc.Texture2D.MipSlice = 0;                             // Which mip are we rendering into?
	rtvDesc.Format = texDesc.Format;                // Same format as texture
	device->CreateRenderTargetView(tex.Get(), &rtvDesc, rtv.GetAddressOf());

	// Create the shader resource view using default options 
	device->CreateShaderResourceView(
		tex.Get(),     // Texture resource itself
		0,                   // Null description = default SRV options
		srv.GetAddressOf()); // ComPtr<ID3D11ShaderResourceView>
}

void Renderer::PreResize() 
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();


}

void Renderer::PostResize(
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
	unsigned int windowWidth, unsigned int windowHeight)
{
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;
	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;
	
	//reset extra post process render targets
	for (int i = 0; i < RenderTargetType::TYPE_COUNT; i++) {
		mRTVs[i].Reset();
		mSRVs[i].Reset();
	}

	//recreate rtvs for ssao
	for (int i = 0; i < RenderTargetType::TYPE_COUNT; i++) {
		if (i == RenderTargetType::SCENE_DEPTH) {
			CreateRenderTarget(windowWidth, windowHeight,
				mRTVs[i], mSRVs[i], DXGI_FORMAT_R32_FLOAT);
		}
		else {
			CreateRenderTarget(windowWidth, windowHeight,
				mRTVs[i], mSRVs[i]);
		}
	}
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
